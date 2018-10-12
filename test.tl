; For Tino :)

(define accumulate
  (lambda (redf v mapf l)
	(foldr redf v (map mapf l))))

(define binswap
  (lambda (f)
	(lambda (a b)
	  (f b a))))

; Tests
(let ((a 1) (b (+ 2 2))) (display a) (display b))
(let* ((a 1) (b (+ a 1))) (display a) (display b))
(cond ((> 0 1) (display 1)) ((< 0 1) (display 2)))
((binswap -) 3 5)
((lambda (x) x) 7)
(define f (lambda (ret) (ret 2) 4))
(f (lambda (x) x))
(call/cc f)
(define mkpair (lambda (a b) (lambda (x) (if x a b))))
(define l (mkpair 1 (mkpair 2 (mkpair 3 #f))))
(l #t)
((l #f) #t)
(((l #f) #f) #t)
