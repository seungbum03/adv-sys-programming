# 고급 시스템 프로그래밍 Advanced System Programming

국민대학교 전자공학부 20113144 심승범
Univ.Kookmin, Electronic Engineering, Shim Seung-Beom

Abstract
- Uploaded most fastest and highly efficient merge code only.
- The average consumed time was 15.49sec.
- The most fastest time was 13.04sec.
- fread() was used for upgrading the performance


File
- gen.c   : n 개의  m MB의 크기의 text 파일 만들기 (file merge용)
- merge.c : merge.c 두개의 파일을 merge 하고 시간을 재는 예제 소스


Usage
- $ make
- $ ./gen 2 100
- $ ./merge /tmp/file_0001 /tmp/file_0002 f_out


