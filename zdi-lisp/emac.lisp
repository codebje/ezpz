(defun emac-setup ()
  (stop)
  (exec #xf3)
  ; reset emac
  (out #x41 #x3f)
  ; set addr
  (out #x25 #x00)
  (out #x26 #x90)
  (out #x27 #x23)
  (out #x28 #xAB)
  (out #x29 #xCD)
  (out #x2a #xEF)
  ; disable ints
  (out #x4c #x00)
  (out #x4d #xff)
  ; buffer size: 64b
  (out #x4b #x80)
  ; pause timeout
  (out #x2b #x14)
  (out #x2c #x00)
  ; mem registers
  (out #x42 #x00)
  (out #x43 #xc0)
  (out #x44 #x00)
  (out #x45 #xc8)
  (out #x47 #x00)
  (out #x48 #xe0)
  (out #x49 #x00)
  (out #x4a #xc8)
  ; status of first packet
  (exec #x01 #x00 #x00 #x00)
  (exec #x21 #x00 #xc0 #xff)
  (exec #xed #x0f)
  (exec #x21 #x03 #xc0 #xff)
  (exec #xed #x0f)
  (exec #x21 #x06 #xc0 #xff)
  (exec #xed #x0f)
  ; poll timer
  (out #x40 #x01)
  ; disable test
  (out #x20 #x00)
  ; config registers
  (out #x21 #x02)
  (out #x22 #x38)
  (out #x23 #x0f)
  (out #x24 #x01)
  (out #x24 #x11)
  ; afr
  (out #x32 #x00) ; addressed to me only
  ; frame length
  (out #x30 #x00)
  (out #x31 #x06)
  ; end reset
  (out #x41 #x00)
  )

(defun mii-setup ()
  ; clkdiv = 20
  (out #x3b #x06)
  ; external PHY address
  (out #x3f #x01)
  ; RGAD=2, PHY_ID1_REG
  (out #x3e #x02)
  ; read register
  (out #x3b #x46)
  )

(defun phyrd (reg)
  (out #x3b #x06)
  (out #x3f #x01)
  (out #x3e reg)
  (out #x3b #x46)
  (loop (if (not (logbitp 7 (in #x50))) (return)))
  (let ((miist (in #x50))
        (istat (in #x4d))
        (prsdl (in #x4e))
        (prsdh (in #x4f)))
    (format t "MIISTATUS=~2,'0x ISTAT=~2,'0x PRSD=~2,'0x~2,'0x~%" miist istat prsdh prsdl))
  (out #x4d #xff))

(defun phywr (reg val)
  (out #x3b #x06)
  (out #x3f #x01)
  (out #x4d #xff)
  (out #x3c (logand #xff val))
  (out #x3d (logand #xff (ash val -8)))
  (out #x3e reg)
  (out #x3b #x86)
  (loop (if (not (logbitp 7 (in #x50))) (return)))
  (let ((miist (in #x50))
        (istat (in #x4d)))
    (format t "MIISTATUS=~2,'0x ISTAT=~2,'0x" miist istat))
  (out #x4d #xff))

