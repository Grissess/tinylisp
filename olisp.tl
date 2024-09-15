; A simple object model library based on coroutine idioms
;
; Depends on std.tl

(define inst-var define)

(define _methods (lambda (meths)
  (map (lambda (meth)
    (apply (lambda (name params . body)
      `((= (cadr #message) (quote ,name))
        (define #reply (car #message))
        (#reply (apply (lambda ,params @body) (cddr #message)))
      )
    ) meth)
  ) meths)
))

(define _default_methods '(
  (#method-missing (method . args) (tl-error `("missing method" ,#this ,method @args)))
))

(define methods (macro meths env
  (tl-eval-in env
    `(begin
      (define #message (current-continuation))
      (if (= (tl-type #message) 'cont)
        (define #this #message)
        #t
      )
      (cond
        ((= (tl-type #message) 'cont)
          #this
        )
        @(_methods meths)
        @(_methods _default_methods)
        (else
          ((car #message) (apply call (cons #this (cons '#method-missing (cdr #message)))))
        )
      )
    )
  )
))

(define call (lambda (obj meth . args)
  (call/cc (lambda (reply) (obj (cons reply (cons meth args)))))
))

(define inst-eval (lambda (obj expr)
  (tl-eval-in (tl-env obj) expr)
))
