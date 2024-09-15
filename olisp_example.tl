; A simple demonstration of object facilities
;
; Depends on olisp.tl

(class new-object (state)
  (inst-var hidden tl-#t)
  (methods
    (print () (display #message))
    (getstate () state)
    (gethidden () hidden)
    (setstate (newstate) (set! state newstate))
    (sethidden (newhidden) (set! hidden newhidden))
    (showenv () (display (if hidden '"(hidden)" (tl-env))))
    (#method-missing (method . args) `("I am method" ,method "with args" @args))
  )
)

(define object (new-object 55))
(define other-object (new-object 'aa))
(display '"class of object" (typenameof object) (typeof object))
(display '"other object state" (call other-object 'getstate))
(display '"old state" (call object 'getstate))
(call object 'setstate 77)
(display '"new state" (call object 'getstate))
(display 'env (call object 'showenv))
(display '"old hidden" (call object 'gethidden))
(call object 'sethidden #f)
(display 'env (call object 'showenv))
(display '"new hidden" (call object 'gethidden))
(display '"state according to inst-eval" (inst-eval object 'state))
(display '"missing method" (call object 'oops 1 2 3))
(display '"other object state" (call other-object 'getstate))
