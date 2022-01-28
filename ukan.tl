; MicroKanren, as in http://webyrd.net/scheme-2013/papers/HemannMuKanren2013.pdf

; We can't use vectors, since TL is too tiny to have them. A special "opaque"
; pair probably suffices.

(define #var '#var)

(define var (lambda (c) (cons #var c)))  ; `(#var . ,c) also works
(define var? (lambda (x) (= (car x) #var)))
(define var=? (lambda (x y) (= (cdr x) (cdr y))))

; The current stdlib "and" doesn't short-circuit--we fix that here.
(define tl-andthen
  (lambda (args env last)
	(if (null? args) last
	  (let ((test (tl-eval-in env (car args))))
		(if test
		  (tl-andthen (cdr args) env test)
		  #f)))))
(define andthen (macro args env (tl-andthen args env #t)))

; RnRS apparently has null? imply (not pair?)--I haven't evaluated that for the
; stdlib yet
(define pair-not-null? (lambda (v) (andthen (pair? v) (not (null? v)))))

(define walk
  (lambda (u s)
	(let ((pr (andthen (var? u) (assocp (lambda (v) (var=? u v)) s))))
	  (if pr (walk (cdr pr) s) u))))
(define ext-s (lambda (x v s) (cons (cons x v) s)))

(define == (lambda (u v)
  (lambda (s/c)
	(let ((s (unify u v (car s/c))))
	  (if s (unit (cons s (cdr s/c))) mzero)))))

(define unit (lambda (x) (cons x mzero)))
(define mzero '())

(define unify
  (lambda (u v s)
	(let ((u (walk u s)) (v (walk v s)))
	  (cond
		((andthen (var? u) (var? v) (var=? u v)) s)
		((var? u) (ext-s u v s))
		((var? v) (ext-s v u s))
		; NB: pair? implies var? but we've exhausted the permutations above
		((and (pair-not-null? u) (pair-not-null? v))
		 (let ((s (unify (car u) (car v) s)))
		   (andthen s (unify (cdr u) (cdr v) s))))
		(else (andthen (= u v) s))))))

(define call/fresh
  (lambda (f)
	(lambda (s/c)
	  (let ((c (cdr s/c)))
		((f (var c)) (cons (car s/c) (+ c 1)))))))

; Definitions ever so slightly modified to be recursive and thus variadic
(define disj (lambda (g1 g2) (lambda (s/c) (mplus (g1 s/c) (g2 s/c)))))
(define conj (lambda (g1 g2) (lambda (s/c) (bind (g1 s/c) g2))))

(define mplus
  (lambda ($1 $2)
	(cond
	  ((null? $1) $2)
	  ((procedure? $1) (lambda () (mplus $2 ($1))))
	  (else (cons (car $1) (mplus (cdr $1) $2))))))

(define bind
  (lambda ($ g)
	(cond
	  ((null? $) mzero)
	  ((procedure? $) (lambda () (bind ($) g)))
	  (else (mplus (g (car $)) (bind (cdr $) g))))))

; Forgive that TL has macro, not define-syntax
(define Zzz
  (macro (g) env
		 (tl-eval-in env `(lambda (s/c) (lambda () (,g s/c))))))
(define conj+
  (macro (first . rest) env
		 (if (null? rest)
		   (tl-eval-in env `(Zzz ,first))
		   (tl-eval-in env `(conj (Zzz ,first) (conj+ @rest))))))
(define disj+
  (macro (first . rest) env
		 (if (null? rest)
		   (tl-eval-in env `(Zzz ,first))
		   (tl-eval-in env `(disj (Zzz ,first) (disj+ @rest))))))
(define conde
  (macro args env
		 (tl-eval-in env `(disj+ @(map (lambda (e) (cons 'conj+ e)) args)))))
(define fresh
  (macro (bindings . body) env
		 (if (null? bindings)
		   (tl-eval-in env `(conj+ @body))
		   (tl-eval-in env `(call/fresh (lambda (,(car bindings)) (fresh ,(cdr bindings) @body)))))))

(define pull
  (lambda ($)
	(if (procedure? $) (pull ($)) $)))
(define take-all
  (lambda ($)
	(let (($ (pull $)))
	  (if (null? $) '() (cons (car $) (take-all (cdr $)))))))
(define take
  (lambda (n $)
	(if (<= n 0) '()
	  (let (($ (pull $)))
		(cond
		  ((null? $) '())
		  (else (cons (car $) (take (- n 1) (cdr $)))))))))

(define mK-reify
  (lambda (s/c*)
	(map reify-state/1st-var s/c*)))
(define reify-state/1st-var
  (lambda (s/c)
	(let ((v (walk* (var 0) (car s/c))))
	  (walk* v (reify-s v '())))))
(define reify-s
  (lambda (v s)
	(cond
	  ((var? v)
	   (let ((n (reify-name (length s))))
		 (cons (cons v n) s)))
	  ((pair-not-null? v) (reify-s (cdr v) (reify-s (car v) s)))
	  (else s))))
(define reify-name (lambda (n) (tl-concat '_. n)))
(define walk*
  (lambda (v s)
	(let ((v (walk v s)))
	  (cond
		((var? v) v)
		((pair-not-null? v) (cons (walk* (car v) s) (walk* (cdr v) s)))
		(else v)))))

(define empty-state '(() . 0))
(define call/empty-state (lambda (g) (g empty-state)))

(define run
  (macro (n args . body) env
		 (tl-eval-in env `(mK-reify (take ,n (call/empty-state (fresh ,args @body)))))))
(define run*
  (macro (args . body) env
		 (tl-eval-in env `(mK-reify (take-all (call/empty-state (fresh ,args @body)))))))
