; Pascal's triangle, from Rosetta Code.

(def genrow (lambda (n l)
  (when (plusp n)
    (print l)
    (genrow (- n 1) (cons 1 (newrow l))))))

(def newrow (lambda (l)
  (if (null (rest l))
      '(1)
      (cons (+ (first l) (second l))
            (newrow (rest l))))))

(def pascal (lambda (n)
  (genrow n '(1))))
