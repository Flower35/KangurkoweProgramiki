#include <stdio.h>
#include <stdint.h>
#include <errno.h>

/**************************************************************/

#define SUCCESS   (0)
#define FAILURE  (-1)

#define FAIL_OPEN  (-2)
#define FAIL_DATA  (-3)
#define FAIL_READ  (-4)
#define FAIL_SEEK  (-5)
#define FAIL_WRITE (-6)

#define TRY_OPEN(__file, __path, __mode) \
  if (NULL == ((* __file) = fopen(__path, __mode))) { \
    perror("fopen()"); \
    return FAIL_OPEN; \
  }

#define SEEK(__where) \
  if (0 != fseek((* file_in), __where, SEEK_SET)) { \
    perror("fseek()"); \
    return FAIL_SEEK; \
  }

#define READ(__var, __size) \
  if (1 != fread(&(__var), __size, 1, (* file_in))) { \
    perror("fread()"); \
    return FAIL_READ; \
  }

#define WRITE_CHECK(__func) \
  if (ferror(* file_out)) { \
    perror(__func); \
    return FAIL_WRITE; \
  }

#define RGB() \
  (0x00ff & x), (0x00ff & (x >> 8)), (0x00ff & (x >> 16))

/**************************************************************/

int bmp2inc
(
  FILE ** file_in,
  FILE ** file_out,
  const char * path_in,
  const char * path_out
)
{
  uint32_t x, width, height, pixels_start, a, b;

  TRY_OPEN(file_in,  path_in,  "rb")
  TRY_OPEN(file_out, path_out, "wb")

  READ(x, 2)
  if (0x4D42 != (0x00ffff & x))
  {
    fprintf(stderr, "Expected BM header!\n");
    return FAIL_DATA;
  }

  SEEK(0x0A)
  READ(pixels_start, 4)

  SEEK(0x12)
  READ(width,  4)
  READ(height, 4)

  READ(x, 4)
  if (0x00180001 != x)
  {
    fprintf(stderr, "Expected 24-bit bitmap!\n");
    return FAIL_DATA;
  }

  fprintf((* file_out), "#define SPLASH_IMAGE_WIDTH   %3u\n", width);
  WRITE_CHECK("fprintf()")

  fprintf((* file_out), "#define SPLASH_IMAGE_HEIGHT  %3u\n", height);
  WRITE_CHECK("fprintf()")

  fprintf((* file_out), "\nconst BYTE SPLASH_PIXELS[3 * SPLASH_IMAGE_WIDTH * SPLASH_IMAGE_HEIGHT] = {\n  ");
  WRITE_CHECK("fprintf()")

  SEEK(pixels_start);

  b = width * height;
  for (a = 0; a < b; a++)
  {
    READ(x, 3)

    fprintf((* file_out), "%u,%u,%u", RGB());
    WRITE_CHECK("fprintf()")

    if (a < (b - 1))
    {
      fputc(',', (* file_out));
      WRITE_CHECK("fputc()")
    }
  }

  fprintf((* file_out), "\n};\n");
  WRITE_CHECK("fprintf()")

  return SUCCESS;
}

/**************************************************************/

int main(int argc, char ** argv)
{
  int test;
  FILE * file_in, * file_out;

  if (3 != argc)
  {
    fprintf(stderr, "Usage: %s [input.bmp] [output.inc]\n", argv[0]);
    return FAILURE;
  }

  file_in = NULL;
  file_out = NULL;
  test = bmp2inc(&(file_in), &(file_out), argv[1], argv[2]);

  if (NULL != file_in)
  {
    fclose(file_in);
  }

  if (NULL != file_out)
  {
    fclose(file_out);
  }

  return test;
}

/**************************************************************/
