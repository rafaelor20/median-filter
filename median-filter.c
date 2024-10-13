#include <png.h>
#include <pngconf.h>
#include <stdio.h>
#include <stdlib.h>

void read_png_file(const char *filename, png_bytep **row_pointers,
                   int *const width, int *const height, png_byte *color_type,
                   png_byte *bit_depth) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    perror("File could not be opened");
    return;
  }

  png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    fclose(fp);
    fprintf(stderr, "png_create_read_struct failed\n");
    return;
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, NULL, NULL);
    fclose(fp);
    fprintf(stderr, "png_create_info_struct failed\n");
    return;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    fprintf(stderr, "Error during init_io\n");
    return;
  }

  png_init_io(png, fp);
  png_read_info(png, info);

  *width = png_get_image_width(png, info);
  *height = png_get_image_height(png, info);
  *color_type = png_get_color_type(png, info);
  *bit_depth = png_get_bit_depth(png, info);

  png_read_update_info(png, info);

  *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * (*height));
  for (int y = 0; y < *height; y++) {
    (*row_pointers)[y] = (png_bytep)malloc(png_get_rowbytes(png, info));
  }

  png_read_image(png, *row_pointers);
  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);
}

void write_png_file(const char *filename, png_bytep *row_pointers, int width,
                    int height, png_byte color_type, png_byte bit_depth) {
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    perror("File could not be opened for writing");
    return;
  }

  png_structp png =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    fclose(fp);
    fprintf(stderr, "png_create_write_struct failed\n");
    return;
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, NULL);
    fclose(fp);
    fprintf(stderr, "png_create_info_struct failed\n");
    return;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    fprintf(stderr, "Error during writing the file\n");
    return;
  }

  png_init_io(png, fp);
  png_set_IHDR(png, info, width, height, bit_depth, color_type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  png_destroy_write_struct(&png, &info);
  fclose(fp);
}

int comp(const void *a, const void *b) {
  png_byte a_temp = *(png_byte *)a;
  png_byte b_temp = *(png_byte *)b;

  if (a_temp < b_temp)
    return -1;
  else if (a_temp == b_temp)
    return 0;
  else
    return 1;
}

void apply_median_filter_row(png_bytep *row_pointers, png_bytep *output_row_pointers,
                             const int width, const int height, const int y,
                             const int window_side) {
  png_byte windowR[window_side * window_side];
  png_byte windowG[window_side * window_side];
  png_byte windowB[window_side * window_side];

  int edgex = window_side / 2;
  int edgey = edgex;

  int num_elem_window = window_side * window_side;
  int median = num_elem_window / 2;

  for (int x = edgex; x < width - edgex; x++) {
    int i = 0;

    // Fill the window with the pixel values in the neighborhood
    for (int fx = 0; fx < window_side; fx++) {
      for (int fy = 0; fy < window_side; fy++) {
        windowR[i] = row_pointers[y + fy - edgey][3 * (x + fx - edgex)];
        windowG[i] = row_pointers[y + fy - edgey][3 * (x + fx - edgex) + 1];
        windowB[i] = row_pointers[y + fy - edgey][3 * (x + fx - edgex) + 2];
        i++;
      }
    }

    // Sort the window arrays
    qsort(windowR, num_elem_window, sizeof(png_byte), comp);
    qsort(windowG, num_elem_window, sizeof(png_byte), comp);
    qsort(windowB, num_elem_window, sizeof(png_byte), comp);

    // Set the median value to the output
    output_row_pointers[y][3 * x] = windowR[median];
    output_row_pointers[y][3 * x + 1] = windowG[median];
    output_row_pointers[y][3 * x + 2] = windowB[median];
  }
}


png_bytep *median_filter(png_bytep *row_pointers, const int width,
                         const int height, const int window_side) {
  int number_of_channels = 3; // Assuming RGB

  // Allocate memory for the output rows
  png_bytep *output_row_pointers =
      (png_bytep *)malloc(sizeof(png_bytep) * height);
  for (int y = 0; y < height; y++) {
    output_row_pointers[y] = (png_bytep)malloc(width * number_of_channels);
  }

  int edgey = window_side / 2;

  // Apply the median filter for each row, avoiding the edges
  for (int y = edgey; y < height - edgey; y++) {
    apply_median_filter_row(row_pointers, output_row_pointers, width, height, y, window_side);
  }

  // Free the original row pointers
  for (int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  return output_row_pointers;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input.png> <output.png>\n", argv[0]);
    return EXIT_FAILURE;
  }

  png_bytep *row_pointers;
  int width, height;
  png_byte color_type, bit_depth;

  read_png_file(argv[1], &row_pointers, &width, &height, &color_type,
                &bit_depth);

  png_bytep *output_row_pointers =
      median_filter(row_pointers, width, height, 5);

  write_png_file(argv[2], output_row_pointers, height, width, color_type,
                 bit_depth);

  // Free the output row pointers
  for (int x = 0; x < width; x++) {
    free(output_row_pointers[x]);
  }
  free(output_row_pointers);

  return EXIT_SUCCESS;
}
