      program example

      implicit none

      include "coda.inc"

      character*1024 filename
      character*32 product_class
      character*32 product_type
      character*32 root_type
      integer pf
      integer cursor
      integer type_class
      integer result
      integer i

      write(*,*) 'Name of the product file:'
      read(*,'(A1024)') filename

      result = coda_init()
      if (result .ne. 0) then
        call handle_coda_error()
      end if

      result = coda_open(filename, pf)
      if (result .ne. 0) then
        call handle_coda_error()
      end if

      result = coda_get_product_class(pf, product_class)
      if (result .ne. 0) then
        call handle_coda_error()
      end if
      write(*,*) 'Product class = ' // product_class

      result = coda_get_product_type(pf, product_type)
      if (result .ne. 0) then
        call handle_coda_error()
      end if
      write(*,*) 'Product type = ' // product_type

      cursor = coda_cursor_new()

      result = coda_cursor_set_product(cursor, pf)
      if (result .ne. 0) then
        call handle_coda_error()
      end if

      result = coda_cursor_get_type_class(cursor, type_class)
      if (result .ne. 0) then
        call handle_coda_error()
      end if

      call coda_type_get_class_name(type_class, root_type)
      write(*,*) 'Root type = ' // root_type

      call coda_cursor_delete(cursor)

      result = coda_close(pf)
      if (result .ne. 0) then
        call handle_coda_error()
      end if

      call coda_done()

      end program


      subroutine handle_coda_error

      implicit none

      include "coda.inc"

      integer err
      character*75 errstr

      err = coda_get_errno()
      call coda_errno_to_string(err, errstr)
      write(*,*) 'Error: ' // errstr
      stop

      end subroutine
