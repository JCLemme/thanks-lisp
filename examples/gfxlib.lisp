; A basic graphics library for the drawing terminal. And also a clock.

(def *disp* (open "/tmp/gfx_pipe" :w))

(def g-line (lambda (x1 y1 x2 y2) (pyprint-to *disp* "line" x1 y1 x2 y2)))
(def g-point (lambda (x y) (pyprint-to *disp* "point" x y)))
(def g-rect (lambda (x y w h) (pyprint-to *disp* "rect" x y w h)))
(def g-frect (lambda (x y w h) (pyprint-to *disp* "fillrect" x y w h)))
(def g-circle (lambda (x y r) (pyprint-to *disp* "circle" x y r)))
(def g-fcircle (lambda (x y r) (pyprint-to *disp* "fillcircle" x y r)))
(def g-text (lambda (x y m) (pyprint-to *disp* "text" x y m)))
(def g-color (lambda (r g b) (pyprint-to *disp* "color" r g b)))
(def g-clear (lambda () (pyprint-to *disp* "clear")))

(def as-rad (lambda (d) (* d (/ 3.1415926 180))))

(def clock-body (lambda (r) (g-circle 120 120 r)))
(def clock-tick (lambda (l1 l2 a) (g-line 
                                          (truncate (+ 120 (*    l1 (sin (as-rad a))))) 
                                          (truncate (+ 120 (* -1 l1 (cos (as-rad a)))))
                                          (truncate (+ 120 (*    l2 (sin (as-rad a))))) 
                                          (truncate (+ 120 (* -1 l2 (cos (as-rad a)))))
                                    )))

(def clock-hand (lambda (l a) (clock-tick 0 l a))) 

; needs better format control
(def clock-print (lambda (x y h m) (pyprint-to *disp* "text" x y "it's" h ":" m)))

(def clock-at (lambda (h m s)
                (let ((ha (* h (/ 360 12)))
                      (ma (* m (/ 360 60)))
                      (sa (* s (/ 360 60))))
                    
                     (progn (g-clear)
                            (g-color 0 0 0)
                            (clock-body 100)
                            (g-color 200 200 200)
                            (dotimes 12 (clock-tick 80 90 (* i (/ 360 12))))
                            (g-color 0 0 0)
                            (clock-hand 65 ha)
                            (clock-hand 90 ma)
                            (g-color 255 0 0)
                            (clock-hand 95 sa)
                            (clock-print 5 220 h m)
                      )
                )
  )
)

(def run-clock (lambda ()
                 (let ((tim 0))
                   (while t
                    (progn
                      (setq tim (decoded-time))
                      (clock-at (car (cdr (cdr tim))) (car (cdr tim)) (car tim))
                      (sleep 1)
                   )
                  )
                 )
  )
)
