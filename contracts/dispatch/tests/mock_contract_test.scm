;; Copyright 2018 Intel Corporation
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

;; -----------------------------------------------------------------
;; -----------------------------------------------------------------
(require "test_common.scm")

(define contract-version (string-append "mock-contract-v" (number->string interface-version)))

;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
(define-class mock-contract-v1
  (instance-vars
   (initialized #f)
   (creator "")
   (value 0)))

(define-method mock-contract-v1 (initialize-instance . args)
  (if (not initialized)
      (begin
        (instance-set! self 'creator (get ':contract 'creator))
        (instance-set! self 'initialized #t))))

(define-const-method mock-contract-v1 (depends contract-id state-hash)
  (put ':ledger 'dependencies (list (list contract-id state-hash)))
  #t)

(define-const-method mock-contract-v1 (get-value)
  (let* ((requestor (get ':message 'originator)))
    (assert (string=? requestor creator) "only the creator can get the value"))
  value)

(define-method mock-contract-v1 (inc-value)
  (let* ((requestor (get ':message 'originator)))
    (assert (string=? requestor creator) "only the creator can inc the value"))
  (instance-set! self 'value (+ value 1))
  value)

;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
(define-class mock-contract-v2
  (class-vars
   (interface-version 2))

  (instance-vars
   (initialized #f)
   (creator "")
   (value 0)))

(define-method mock-contract-v2 (initialize-instance . args)
  (if (not initialized)
      (let* ((environment (car args))
             (requestor (send environment 'get-originator-id)))
        (instance-set! self 'creator requestor)
        (instance-set! self 'initialized #t)))
  #t)


(define-const-method mock-contract-v2 (get-value environment request response)
  (let ((requestor (send environment 'get-originator-id)))
    (assert (string=? requestor creator) "only the creator can get the value"))
  (send response 'return-value value #f))

(define-method mock-contract-v2 (inc-value environment request response)
  (let ((requestor (send environment 'get-originator-id)))
    (assert (string=? requestor creator) "only the creator can inc the value"))
  (instance-set! self 'value (+ value 1))
  (send response 'return-value value #t))

(define-method mock-contract-v2 (depends environment request response)
  (let ((dependencies (send request 'get-parameter "depends" utility-package::contract-state-dependency?)))
    ;;(map (lambda (d) (send response 'add-dependency d)) (vector->list dependencies))
    (map (lambda (d) (send response 'add-dependency d)) dependencies)
    (send response 'return-success #f)))

;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
(define environment-sexpr
  `(("ContractID" "")
    ("CreatorID" "a")
    ("OriginatorID" "a")
    ("StateHash" "")
    ("MessageHash" "")
    ("ContractCodeName" ,contract-version)))

(define environment-sexpr-bad
  `(("ContractID" "")
    ("CreatorID" "a")
    ("OriginatorID" "b")
    ("StateHash" "")
    ("MessageHash" "")
    ("ContractCodeName" ,contract-version)))

(define invoke-inc-sexpr
  `(("Method" "inc-value")
    ("PositionalParameters" #())
    ("KeywordParameters" ())))

(define invoke-get-sexpr
  `(("Method" "get-value")
    ("PositionalParameters" #())
    ("KeywordParameters" ())))

(define invoke-depends-sexpr
  `(("Method" "depends")
    ("PositionalParameters" #("contract1" "state-hash1"))
    ("KeywordParameters"
     (("depends"
       (("contract1" "state-hash-1")
        ("contract2" "state-hash-2")))))))

(define invoke-dec-sexpr
  `(("Method" "dec-value")
    ("PositionalParameters" #())
    ("KeywordParameters" ())))

(test-logger::highlight (string-append "START TEST [" (number->string interface-version) "]"))

(catch-successful-test (**dispatch** "bad json" "bad json") "failed to catch ill-formed JSON")
(catch-failed-test (execute-initialize environment-sexpr check-status?))
(catch-failed-test (execute-dispatch environment-sexpr invoke-inc-sexpr (check-value-generator 1)))
(catch-failed-test (execute-dispatch environment-sexpr invoke-inc-sexpr (check-value-generator 2)))
(catch-failed-test (execute-dispatch environment-sexpr invoke-inc-sexpr (check-value-generator 3)))
(catch-failed-test (execute-dispatch environment-sexpr-bad invoke-inc-sexpr check-failed-status?))
(catch-failed-test (execute-dispatch environment-sexpr invoke-get-sexpr (check-value-generator 3)))
(catch-failed-test (execute-dispatch environment-sexpr-bad invoke-get-sexpr check-failed-status?))
(catch-failed-test (execute-dispatch environment-sexpr invoke-depends-sexpr check-status?))
(catch-failed-test (execute-dispatch environment-sexpr invoke-dec-sexpr check-failed-status?))

(test-logger::highlight "TEST COMPLETED SUCCESSFULLY")
(quit 0)
