/* 시스템 호출에 데이터를 전달할 때 한 가상 페이지에서 다른 페이지로
  넘어가는 데이터를 사용하여 시스템 호출을 깨뜨리려고 시도하는 테스트를 위한 유틸리티 함수입니다. */

#include <inttypes.h>
#include <round.h>
#include <string.h>
#include "tests/userprog/boundary.h"

static char dst[8192];

/* 페이지의 시작 부분을 반환합니다. 반환된 포인터의 양쪽에 최소 2048개의 수정 가능한 바이트가 있습니다. */
void *
get_boundary_area (void) 
{
  char *p = (char *) ROUND_UP ((uintptr_t) dst, 4096);
  if (p - dst < 2048)
    p += 4096;
  return p;
}

/* SRC의 복사본을 두 페이지의 경계에 걸쳐 반환합니다. */
char *
copy_string_across_boundary (const char *src) 
{
  char *p = get_boundary_area ();
  p -= strlen (src) < 4096 ? strlen (src) / 2 : 4096;
  strlcpy (p, src, 4096);
  return p;
}

