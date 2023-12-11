/* See LICENSE for license details. */

OUTPUT_ARCH( "riscv" )

ENTRY( boot_decide_vector )

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  /* Beginning of code and text segment */
  . = 0x80000000;

  /* restore: Restore Program code section */
  .restore.code :
  {
    *(.restore.code)
  }

  . = ALIGN(0x1000);
  .restore.data :
  {
    *(.restore.data)
  }

  . = 0x80100000;
  .payload :
  {
    PROVIDE(_payload_start = .);
    *(.payload)
    . = ALIGN(8);
    PROVIDE(_payload_end = .);
  }
}
