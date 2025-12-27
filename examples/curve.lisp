; Curve plotter borrowed from uLisp.
; A number of changes have been made to accomodate thanks-lisp's 
; borked standard library.

(load "examples/gfxlib.lisp")

(def rgb (lambda (r g b) (list r g b)))

(def point (lambda (x y c) 
             (progn 
               (g-color (car c) (car (cdr c)) (car (cdr (cdr c))))
               (g-point x (- 239 y))
             )
            )
)

(def sqfun (lambda (r) (+ (* 2.5 (- 1 r) (- 1 r)) (* 2 r r 0.7) -1.5)))
(def sinfun (lambda (r) (* (- r 1) (sin (* r 24)))))

(def plot (lambda (fun)
  (let ((m 0) (n 0))
    (progn (g-clear)
    (let-dotimes x 120 (progn
      (sleep 0.001) ; to let emscripten service events
      (let ((w (truncate (sqrt (- (* 120 120) (* x x))))))
        (let-dotimes v (* 2 w)
          (let ((z (- v w))
                 (r (/ (sqrt (+ (* x x) (* z z))) 120))
                 (q (fun r))
                 (y (truncate (+ (/ z 3) (* q 120))))
                 (c (rgb (truncate (* r 255)) (truncate (* (- 1 r) 255)) 128)))
            (when
                (cond
                 ((== 0 v) (progn (setq m y) (setq n y)))
                 ((> y m) (setq m y))
                 ((< y n) (setq n y))
                 (t nil))
              (point (- 120 x) (+ 120 y) c) 
              (point (+ 120 x) (+ 120 y) c)))))))))))
