; Essential combinatorics

(define map
  (lambda (f l)
	(if (null? l)
	  '()
	  (cons (f (car l)) (map f (cdr l))))))

(define foldr
  (lambda (f v l)
	(if (null? l)
	  v
	  (foldr f (f v (car l)) (cdr l)))))

(define foldl
  (lambda (f v l)
	(if (null? l)
	  v
	  (f (car l) (foldl f v (cdr l))))))

; Silly aliases

(define cadr
  (lambda (l)
	(car (cdr l))))

(define caddr
  (lambda (l)
	(car (cdr (cdr l)))))

(define cadddr
  (lambda (l)
	(car (cdr (cdr (cdr l))))))

(define caar
  (lambda (l)
	(car (car l))))

(define caaar
  (lambda (l)
	(car (car (car l)))))

(define caaaar
  (lambda (l)
	(car (car (car (car l))))))

(define cdar
  (lambda (l)
	(cdr (car l))))

; Language essentials

(define last
  (lambda (l)
	(if (null? (cdr l))
	  (car l)
	  (last (cdr l)))))

(define begin
  (macro (rest)
		 (last (map eval rest))))

(define list
  (macro (rest)
		 (foldl cons '() (map eval rest))))

(define let
  (macro (bindings body)
		 (eval (cons (cons 'lambda (cons (map car bindings) body)) (map cadr bindings)))))

(define let*
  (macro (bindings body)
		 (if (null? bindings)
		   (eval (cons 'begin body))
		   (eval (cons 'let (cons (list (car bindings)) (list (cons 'let* (cons (cdr bindings) body)))))))))

(define cond
  (macro (clauses)
		 (if (eval (caar clauses))
		   (eval (cons 'begin (cdar clauses)))
		   (eval (cons 'cond (cdr clauses))))))

; Mathematics and Logic

(define not
  (lambda (a)
	(nand a a)))

(define and
  (lambda (a b)
	(not (nand a b))))

(define or
  (lambda (a b)
	(nand (not a) (not b))))

(define <=
  (lambda (a b)
	(or
	  (< a b)
	  (= a b))))

(define >
  (lambda (a b)
	(not (<= a b))))

(define >=
  (lambda (a b)
	(or
	  (> a b)
	  (= a b))))

(define max
  (lambda (a b)
	(if (> a b)
	  a
	  b)))

(define min
  (lambda (a b)
	(if (< a b)
	  a
	  b)))

; List manipulation

(define length
  (lambda (l)
	(foldr + 0 (map (lambda (x) 1) l))))

(define item
  (lambda (l i)
	(cond
	  ((< i 0) '())
	  ((= i 0) (car l))
	  ((null? l) '())
	  ((> i 0) (item (cdr l) (- i 1))))))

; For Tino :)

(define accumulate
  (lambda (redf v mapf l)
	(foldr redf v (map mapf l))))

; Tests
(let ((a 1) (b (+ 2 2))) (display a) (display b))
(let* ((a 1) (b (+ a 1))) (display a) (display b))
(cond ((> 0 1) (display 1)) ((< 0 1) (display 2)))
