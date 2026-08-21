(define puts (lambda (sym)
               (if (= 0 (tl-length sym))
                 #t
                 (begin
                   (tl-writec (tl-ord sym 0))
                   (puts (tl-substr sym 1))))))

(define rc (lambda ()
             (let ((char (tl-readc)))
               (if (or* (= 10 char) (= 13 char) (= 32 char))
                 (rc)
                 char))))

(define string (lambda (accum) 
                 (let ((char (rc)))
                   (if (= 39 char)
                     accum
                     (string (tl-concat accum (tl-chr char)))))))

(define lit (lambda ()
              (let ((first (rc)))
                (if (= 39 first)
                  (string '"")
                  (begin
                    (tl-putbackc first)
                    (tl-read))))))

(define fromdig (lambda (accum sym)
                  (let (
                        (dig (- (tl-ord sym 0) (tl-ord '"0" 0)))
                        (next (tl-substr sym 1)))
                    (if (= (tl-length next) 0)
                      (+ accum dig)
                      (fromdig (* 10 (+ accum dig)) next)))))

(define toint (lambda (val)
                (if (= (tl-type val) 'int)
                  val
                  (fromdig 0 val))))

(define expr (lambda (accum)
               (let ((op (tl-chr (rc))))
                 (cond
                   ((= op '";") accum)
                   (else
                     (let ((rhs (lit)))
                       (cond
                         ((and (= op '+) (= (tl-type accum) 'sym)) (expr (tl-concat accum rhs)))
                         ((and (= op '+) (= (tl-type accum) 'int)) (expr (+ accum (toint rhs))))
                         ((= op '-) (expr (- (toint accum) (toint rhs))))
                         ((= op '/) (expr (/ (toint accum) (toint rhs))))
                         ((= op '*) (expr (* (toint accum) (toint rhs))))
                         (else (error (tl-concat '"syntax error you nub: accum " accum '" op " op))))))))))


(define dojs (lambda ()
               (puts '"> ")
               (display (expr (lit)))
               (dojs)))

