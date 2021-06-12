; Unprefixing

(tl-define define tl-define)

(define #t tl-#t)
(define #f tl-#f)

(define error tl-error)
(define lambda tl-lambda)
(define macro tl-macro)
(define apply tl-apply)
(define set! tl-set!)
(define display tl-display)
(define if tl-if)

(define + tl-+)
(define - tl--)
(define * tl-*)
(define / tl-/)
(define % tl-%)

(define = tl-=)
(define < tl-<)

(define nand tl-nand)

; TODO: replace these with clever usages of formal specifiers
(define car tl-car)
(define cdr tl-cdr)
(define cons tl-cons)

(define type tl-type)
(define null? tl-null?)

;(define apply tl-apply)
(define call-with-current-continuation tl-call-with-current-continuation)
(define call/cc call-with-current-continuation)

(define read tl-read)

; Parser init

(tl-prefix "'" quote)
(define quote
  (macro (lit) environ lit))

; Continuation compatibility
(define tl-eval-in
  (lambda (env ex)
	(call/cc (lambda (ret) (tl-eval-in& env ex ret)))))

(define current-continuation
  (lambda () (call/cc (lambda (cc) (cc cc)))))

; Essential combinatorics

(define map
  (lambda (f l)
	(if (null? l)
	  '()
	  (cons (f (car l)) (map f (cdr l))))))

(define for-each
  (lambda (f l)
	(if (null? l)
	  #t
	  (begin
		(f (car l))
		(for-each f (cdr l))))))

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

; Core language

(define pair?
  (lambda (v)
	(= (type v) 'pair)))

(define append
  (lambda (pref suff)
	(if (null? pref) suff
	  (if (null? (cdr pref))
		(cons (car pref) suff)
		(cons (car pref) (append (cdr pref) suff))))))

(define reverse
  (lambda (l)
	(define res '())
	(for-each (lambda (e) (set! res (cons e res))) l)
	res))

(tl-prefix "," unquote)
(tl-prefix "@" unquote-splicing)
(tl-prefix "`" quasiquote)
(define tl-quasiquote
  (lambda (l env)
	(if (null? l)
	  '()
	  (if (pair? (car l))
		(if (= (caar l) 'unquote)
		  (cons (tl-eval-in env (cadar l)) (tl-quasiquote (cdr l) env))
		  (if (= (caar l) 'unquote-splicing)
			(append (tl-eval-in env (cadar l)) (tl-quasiquote (cdr l) env))
			(cons (tl-quasiquote (car l) env) (tl-quasiquote (cdr l) env))))
		(cons (car l) (tl-quasiquote (cdr l) env))))))

(define quasiquote
  (macro (ex) environ
		 (tl-quasiquote ex environ)))

; Silly aliases

(define eval
  (macro (expr) environ
		 (tl-eval-in& environ expr (lambda (ex) (tl-eval-in environ ex)))))

(define caar
  (lambda (l)
	(car (car l))))

(define cadr
  (lambda (l)
	(car (cdr l))))

(define cdar
  (lambda (l)
	(cdr (car l))))

(define cddr
  (lambda (l)
	(cdr (cdr l))))

(define caaar
  (lambda (l)
	(car (car (car l)))))

(define caadr
  (lambda (l)
	(car (car (cdr l)))))

(define cadar
  (lambda (l)
	(car (cdr (car l)))))

(define caddr
  (lambda (l)
	(car (cdr (cdr l)))))

(define cdaar
  (lambda (l)
	(cdr (car (car l)))))

(define cdadr
  (lambda (l)
	(cdr (car (cdr l)))))

(define cddar
  (lambda (l)
	(cdr (cdr (car l)))))

(define cdddr
  (lambda (l)
	(cdr (cdr (cdr l)))))

(define caaaar
  (lambda (l)
	(car (car (car (car l))))))

(define caaadr
  (lambda (l)
	(car (car (car (cdr l))))))

(define caadar
  (lambda (l)
	(car (car (cdr (car l))))))

(define caaddr
  (lambda (l)
	(car (car (cdr (cdr l))))))

(define cadaar
  (lambda (l)
	(car (cdr (car (car l))))))

(define cadadr
  (lambda (l)
	(car (cdr (car (cdr l))))))

(define caddar
  (lambda (l)
	(car (cdr (cdr (car l))))))

(define cadddr
  (lambda (l)
	(car (cdr (cdr (cdr l))))))

(define cdaaar
  (lambda (l)
	(cdr (car (car (car l))))))

(define cdaadr
  (lambda (l)
	(cdr (car (car (cdr l))))))

(define cdadar
  (lambda (l)
	(cdr (car (cdr (car l))))))

(define cdaddr
  (lambda (l)
	(cdr (car (cdr (cdr l))))))

(define cddaar
  (lambda (l)
	(cdr (cdr (car (car l))))))

(define cddadr
  (lambda (l)
	(cdr (cdr (car (cdr l))))))

(define cdddar
  (lambda (l)
	(cdr (cdr (cdr (car l))))))

(define cddddr
  (lambda (l)
	(cdr (cdr (cdr (cdr l))))))

; Language essentials

(define list
  (lambda l l))

(define begin
  (macro body environ
		 (tl-eval-in environ `((lambda () @body)))))
(define cond
  (macro clauses environ
		 (if (tl-eval-in environ (caar clauses))
		   (tl-eval-in environ (cons 'begin (cdar clauses)))
		   (tl-eval-in environ (cons 'cond (cdr clauses))))))

(define else #t)

(define let
  (macro (bindings . body) environ
		 (tl-eval-in environ `((lambda ,(map car bindings) @body) @(map cadr bindings)))))

(define let*
  (macro (bindings . body) environ
		 (tl-eval-in environ (if (null? bindings)
							   `(begin @body)
							   `(let (,(car bindings)) (let* ,(cdr bindings) @body))))))

;(define let*
;  (macro (bindings body)
;		 (if (null? bindings)
;		   (cons 'begin body)
;		   (cons (car bindings) (cons (cdr bindings) '())))))


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
	  (else (item (cdr l) (- i 1))))))

; Useful reducers

(define any
  (lambda (f l)
	(foldr or #f (map f l))))

(define all
  (lambda (f l)
	(foldr and #t (map f l))))

(define id
  (lambda (x) x))

; Associative list manipulation

(define assoc
  (lambda (k a)
	(cond
	  ((null? a) #f)
	  ((= k (caar a)) (car a))
	  (else (assoc k (cdr a))))))

(define del-assoc
  (lambda (k a)
	(cond
	  ((null? a) '())
	  ((= k (caar a)) (del-assoc k (cdr a)))
	  (else (cons (car a) (del-assoc k (cdr a)))))))

; loop/recur

(define tl-loop-co-sym '#tl-loop)

(define loop
  (macro (bindings . body) env
		 (tl-eval-in env `(apply (lambda ,(cons tl-loop-co-sym (map car bindings))
								   @body
								   )
								 (call/cc (lambda (co) (cons co (quote ,(map cadr bindings)))))))))

(define recur
  (macro args env
		 (tl-eval-in env
					 `((eval ,tl-loop-co-sym) (list (eval ,tl-loop-co-sym) @args)))))
