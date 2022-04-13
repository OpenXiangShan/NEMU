/* See LICENSE for license details. */

OUTPUT_ARCH( "riscv" )

ENTRY( boot_decide_vector )

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  /* Begining of code and text segment */
  . = 0x80000000;

  /* restore: Restore Program code section */
  .restore.code :
  {
    *(.restore.code)
  }
  . = 0x80000f00;
  .restore.flag :
  {
    *(.restore.flag)
  }
  . = 0x80001000;
  .restore.data :
  {
    *(.restore.data)
  }
}
