;;;
(defun hexb (x) (format t "~2,'0x" x))

(defun in0 (port)
  (zdi-exec #xed #x08 port)
  (logand #xff (zdi-rw-read #x01)))

(defun out0 (port val)
  (zdi-exec #x3e val)
  (zdi-exec #xed #x39 port))

(defun zdi-break () (zdi! #x10 #x80))
(defun zdi-cont () (zdi! #x10 #x00))

(defun zdi-exec (op1 &optional op2 op3 op4 op5)
  (if op5 (zdi! #x21 op5) ())
  (if op4 (zdi! #x22 op4) ())
  (if op3 (zdi! #x23 op3) ())
  (if op2 (zdi! #x24 op2) ())
  (zdi! #x25 op1))

(defun zdi-rw-read (var)
  (when (>= 7 var 0)
    (zdi! #x16 var)
    (logior (ash (zdi? #x12) 16)
            (ash (zdi? #x11) 8)
            (zdi? #x10))))

(defun zdi-status ()
  (let ((stat (zdi? #x03))
        (bus (zdi? #x17)))
    (format t "ZDI_ACTIVE=~3a  HALT_SLP=~3a  ADL=~3a  MADL=~3a  EIF=~3a  ZDI_BUSACK=~3a  ZDI_BUS_STAT=~a"
            (logbitp 7 stat)
            (logbitp 5 stat)
            (logbitp 4 stat)
            (logbitp 3 stat)
            (logbitp 2 stat)
            (logbitp 7 bus)
            (logbitp 6 bus))))

(defun ram-setup ()
  (out0 #xa8 #x00)
  (out0 #xa9 #x07)
  (out0 #xaa #x28)
  (out0 #xf7 #x08))

(defun ram-write (addr val)
  (zdi! #x13 (logand #xff addr))
  (zdi! #x14 (logand #xff (ash addr -8)))
  (zdi! #x15 (logand #xff (ash addr -16)))
  (zdi! #x16 #x87)
  (zdi! #x30 val))

(defun ram-read (addr)
  (zdi! #x13 (logand #xff addr))
  (zdi! #x14 (logand #xff (ash addr -8)))
  (zdi! #x15 (logand #xff (ash addr -16)))
  (zdi! #x16 #x87)
  (zdi! #x16 #x0b)
  (zdi? #x10))

(defun ramchck ()
  (zdi! #x16 #x08)
  (ram-setup)
  (dotimes (shift 12)
    (dotimes (addr 256) (ram-write (+ #x40000 (ash addr shift)) addr))
    (dotimes (addr 256)
      (let ((val (ram-read (+ #x40000 (ash addr shift)))))
        (if (/= val addr) (format t "bad read at ~6,'0x expected ~2,'0x got ~2,'0x~%" (ash addr shift) addr val))))))

(defun mem (addr)
  (dotimes (x 16) (hexb (ram-read (+ addr x)))))

  (zdi! #x16 #x08)
  (zdi! #x13 #x00)
  (zdi! #x14 #x00)
  (zdi! #x15 #x07)
  (zdi! #x16 #x87)
  (mapc (lambda (x) (zdi! #x30 x))
        (list
         #xed #x18 #xfb
         #x01 #x49 #xb6 #x00
         #xed #x01 #xf5 #xed #x09 #xf5 #x3e #xff #xed #x39 #xf9
         #xed #x01 #xf5 #xed #x09 #xf5 #x3e #x00 #xed #x39 #xfa
         #x3e #x01 #xed #x39 #xff
         #xed #x10 #xfb #xcb #x6a #x28 #xf9
         #xd5
         #x11 #x00 #x00 #x08
         #x21 #x00 #x00 #x00
         #x01 #x00 #x58 #x01
         #xed #xb0
         #xd1
         #xed #x28 #xfb #xcb #x6d #x28 #xf9
         #x01 #x49 #xb6 #x00
         #xed #x01 #xf5 #xed #x09 #xf5 #x3e #xff
         #xed #x39 #xfa
         #x18 #xfe))
  (zdi! #x13 #x00)
  (zdi! #x14 #x00)
  (zdi! #x15 #x07)
  (zdi! #x16 #x87)

(defun erase()
  (zdi-exec #x01 #x49 #xb6 #x00)
  (zdi-exec #xed #x01 #xf5)
  (zdi-exec #xed #x09 #xf5)
  (zdi-exec #x3e #xff)
  (zdi-exec #xed #x39 #xf9)
  (zdi-exec #xed #x01 #xf5)
  (zdi-exec #xed #x09 #xf5)
  (zdi-exec #x3e #x00)
  (zdi-exec #xed #x39 #xfa)
  (zdi-exec #x3e #x01)
  (zdi-exec #xed #x39 #xff))

