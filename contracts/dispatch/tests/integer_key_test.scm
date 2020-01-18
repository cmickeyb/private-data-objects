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
(require "integer-key.scm")

;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
(define environment-sexpr-id1
  `(("ContractID" "contract-id-000")
    ("CreatorID" "creator-id-111")
    ("OriginatorID" "originator-id-222")
    ("StateHash" "state-hash-333")
    ("MessageHash" "message-hash-444")
    ("ContractCodeName" "integer-key")))

(define environment-sexpr-id2
  `(("ContractID" "contract-id-000")
    ("CreatorID" "creator-id-111")
    ("OriginatorID" "originator-id-333")
    ("StateHash" "state-hash-333")
    ("MessageHash" "message-hash-444")
    ("ContractCodeName" "integer-key")))

(define invoke-create-sexpr
  `(("Method" "create")
    ("PositionalParameters" #("test-key-a" 5))
    ("KeywordParameters" ())))

(define invoke-inc-sexpr
  `(("Method" "inc")
    ("PositionalParameters" #("test-key-a" 2))
    ("KeywordParameters" ())))

(define invoke-get-sexpr
  `(("Method" "get-value")
    ("PositionalParameters" #("test-key-a"))
    ("KeywordParameters" ())))

(define invoke-escrow-sexpr
  `(("Method" "escrow")
    ("PositionalParameters" ,(vector "test-key-a" agent-public-key))
    ("KeywordParameters" ())))

(define invoke-escrow-attestation-sexpr
  `(("Method" "escrow-attestation")
    ("PositionalParameters" #("test-key-a"))
    ("KeywordParameters" ())))

(define (invoke-disburse-sexpr dependencies signature)
  (list (list "Method" "disburse")
        (list "PositionalParameters" (vector "test-key-a" dependencies signature))
        (list "KeywordParameters" '())))

(test-logger::highlight (string-append "START TEST"))

(catch-failed-test (execute-initialize environment-sexpr-id1 check-status?))
(catch-failed-test (execute-dispatch environment-sexpr-id1 invoke-create-sexpr check-status?))
(catch-failed-test (execute-dispatch environment-sexpr-id1 invoke-inc-sexpr check-status?))
(catch-failed-test (execute-dispatch environment-sexpr-id1 invoke-inc-sexpr check-status?))
(catch-failed-test (execute-dispatch environment-sexpr-id1 invoke-inc-sexpr check-status?))
(catch-failed-test (execute-dispatch environment-sexpr-id1 invoke-get-sexpr (check-value-generator 11)))
(catch-failed-test (execute-dispatch environment-sexpr-id1 invoke-escrow-sexpr check-status?))
(catch-failed-test
 (let* ((attestation-expr (execute-dispatch environment-sexpr-id1 invoke-escrow-attestation-sexpr check-status?))
        (counter (eval `(make-instance escrow-counter ,@(car attestation-expr))))
        (dependencies (cadr attestation-expr))
        (signature (caddr attestation-expr)))
   (let ((agent-signature (send agent-keys 'sign-expression (list (send counter 'externalize) dependencies))))
     (execute-dispatch environment-sexpr-id1 (invoke-disburse-sexpr dependencies agent-signature) check-status?))))

(test-logger::highlight "TEST COMPLETED SUCCESSFULLY")
(quit 0)
