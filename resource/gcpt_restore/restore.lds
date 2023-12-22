/* See LICENSE for license details. */

OUTPUT_ARCH( "riscv" )

ENTRY( place_of_restore_size )

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  /* Beginning of code and text segment */
  . = 0x80000000;
  gcpt_begin = .;

  .restore.size :
  {
    *(.restore.size)
  }

  /* restore: Restore Program code section */
  .restore.code :
  {
    *(.restore.code)
  }

  . = ALIGN(0x400);
  .restore.flag :
  {
    *(.restore.flag)
  }

  gcpt_end = .;

  . = 0x80100000;
  .payload :
  {
    PROVIDE(_payload_start = .);
    *(.payload)
    . = ALIGN(8);
    PROVIDE(_payload_end = .);
  }
}
