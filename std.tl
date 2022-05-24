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

(tl-prefix "~" tl-negate)
(define tl-negate (lambda (x) (- 0 x)))

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

(define filter
  (lambda (f l)
	(if (null? l)
	  '()
	  (if (f (car l))
		(cons (car l) (filter f (cdr l)))
		(filter f (cdr l))))))

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

(define procedure?
  (lambda (v)
	(or (= (type v) 'func) (= (type v) 'macro))))

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

(define tl-permute-inner
  (lambda (choices f pfx)
	(if (null? choices)
	  (f pfx)
	  (for-each
		(lambda (e) (tl-permute-inner (cdr choices) f (cons e pfx)))
		(car choices)))))
(define tl-permute (lambda (choices f) (tl-permute-inner choices f '())))

(define tl-path-get
  (lambda (l path)
	(cond
	  ((null? path) l)
	  ((= (car path) 'a) (tl-path-get (car l) (cdr path)))
	  ((= (car path) 'd) (tl-path-get (cdr l) (cdr path)))
	  ((= (car path) 'i) (tl-path-get l (cdr path))))))
(define tl-make-path
  (lambda (var path)
	(cond
	  ((null? path) var)
	  ((= (car path) 'a) (cons 'car (cons (tl-make-path var (cdr path)) '())))
	  ((= (car path) 'd) (cons 'cdr (cons (tl-make-path var (cdr path)) '())))
	  ((= (car path) 'i) (tl-make-path var (cdr path))))))
(define tl-make-cxr
  (lambda (perm)
	(display 'tl-make-cxr perm)
	(let* (
		   (conc (apply tl-concat (filter (lambda (x) (not (= x 'i))) perm)))
		   (sym (tl-concat 'c conc 'r))
		   )
	  (eval `(set! ,sym (lambda (l) ,(tl-make-path 'l perm)))))))

(define tl-if-defined
  (macro (var iftrue iffalse) env
		 (if (= (tl-eval-in env `(tl-rescue (lambda () ,var #t))) #t)
		   (tl-eval-in env iftrue)
		   (tl-eval-in env iffalse))))

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

; Need these four for bootstrapping
(define caar (lambda (l) (car (car l))))
(define cdar (lambda (l) (cdr (car l))))
(define cadr (lambda (l) (car (cdr l))))
(define cadar (lambda (l) (car (cdr (car l)))))
(tl-if-defined no-cxr
			   #t
			   (tl-permute '((a d) (a d) (a d i) (a d i)) tl-make-cxr))

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

(define assocp
  (lambda (pred a)
	(cond
	  ((null? a) #f)
	  ((pred (caar a)) (car a))
	  (else (assocp pred (cdr a))))))

(define del-assocp
  (lambda (pred a)
	(cond
	  ((null? a) '())
	  ((pred (caar a)) (del-assocp pred (cdr a)))
	  (else (cons (car a) (del-assocp pred (cdr a)))))))

(define assoc
  (lambda (k a)
	(assocp (lambda (x) (= k x)) a)))

(define del-assoc
  (lambda (k a)
	(del-assocp (lambda (x) (= k x)) a)))

; loop/recur

(define tl-loop-co-sym '#tl-continue)
(define tl-loop-br-sym '#tl-break)

(define loop
  (macro (bindings . body) env
		 (tl-eval-in env `(call/cc (lambda (,tl-loop-br-sym)
									 (apply (lambda ,(cons tl-loop-co-sym (map car bindings))
											  @body
											  )
											(call/cc (lambda (co) (cons co (quote ,(map cadr bindings)))))))))))

(define recur
  (macro args env
		 (tl-eval-in env
					 `((eval ,tl-loop-co-sym) (list (eval ,tl-loop-co-sym) @args)))))

(define break
  (macro (value) env
		 (tl-eval-in env `((eval ,tl-loop-br-sym) ,value))))
