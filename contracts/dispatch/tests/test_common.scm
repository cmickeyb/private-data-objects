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
(require "debug.scm")
(require "utility.scm")
(require "signing-keys.scm")

;; Add "version" parameter to the command line arguments when
;; running the test to set version number to 1 or 2
(define convert-result string->expression)
(define interface-version 1)

(let ((version (member "version" *args*)))
  (if (and version (= (string->number (cadr version)) 2))
      (begin
        (set! convert-result (lambda (r) r))
        (set! interface-version 2))))

;; Set the loglevel
(define *log-level* test-logger::DEBUG)
(let ((loglevel (member "loglevel" *args*)))
  (if loglevel (set! *log-level* (string->number (cadr loglevel)))))

(test-logger::set-log-level *log-level*)

;; Set up the persistent store
(catch handle-failed-test
       (let ((result (key-value-open "dispatch-test.mdb")))
         (if (eq? result '**pdo-error**) (throw **pdo-error**))))

(let ((builtin_quit quit))
  (define (quit status)
    (begin (key-value-close) (builtin_quit status))))

(define agent-keys (make-instance signing-keys))
(define agent-public-key (send agent-keys 'get-public-signing-key))

;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;; XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
(define (check-value-generator value)
  (lambda (result-json)
    (let ((result-expr (json-to-expression result-json)))
      (let ((status (cadr (assoc "Status" result-expr)))
            (result (cadr (assoc "Result" result-expr))))
        (test-logger::logger-debug "result: " result-expr)
        (and status (= (convert-result result) value))))))

(define (check-status? result-json)
  (let ((result-expr (json-to-expression result-json)))
    (cadr (assoc "Status" result-expr))))

(define (check-failed-status? result-json)
  (let ((result-expr (json-to-expression result-json)))
    (not (cadr (assoc "Status" result-expr)))))

(define (execute-initialize env success?)
  (test-logger::logger-debug "execute-initialize <start>: " env)
  (let ((environment-json (expression-to-json env)))
    (assert (string? environment-json) "bad environment")
    (let ((result (**initialize** environment-json)))
      (assert (success? result) "test failed, unexpected result: " result)
      (test-logger::logger-debug "execute-initialize <return>: " result))))

(define (execute-dispatch env inv success?)
  (test-logger::logger-debug "execute-dispatch <start>: " env inv)
  (let ((invocation-json (expression-to-json inv))
        (environment-json (expression-to-json env)))
    (assert (string? invocation-json) "bad invocation")
    (assert (string? environment-json) "bad environment")
    (let* ((result (**dispatch** environment-json invocation-json))
           (result-sexpr (json-to-expression result)))
      (assert (success? result) "test failed, unexpected result: " result)
      (test-logger::logger-debug "execute-dispatch <return>: " result)
      (let ((value (cadr (assoc "Result" result-sexpr))))
        (convert-result value)))))
