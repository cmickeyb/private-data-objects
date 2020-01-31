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

;;
;; asset-type.scm
;;
;; Define the contract class for an asset type. Very simple contract
;; that is primarily used to create a registered identifier for the
;; asset type.

(define-macro (assert pred . message)
  `(if (not ,pred) (throw ,@message)))

;; =================================================================
;; CLASS: asset-type
;; =================================================================
(define-class asset-type-contract
  (class-vars
   (interface-version 2))

  (instance-vars
   (initialized #f)
   (asset-type-initialized #f)
   (name "")
   (description "")
   (link "")
   (creator "")))

(define-method asset-type-contract (initialize-instance . args)
  (if (not initialized)
      (let* ((environment (car args)))
        (instance-set! self 'creator (send environment 'get-originator-id))
        (instance-set! self 'initialized #t))))

;; -----------------------------------------------------------------
;; NAME: initialize
;;
;; DESCRIPTION: set the name, description and link fields in the
;; asset type, this is not
;;
;; PARAMETERS:
;;     name -- short name, string 32 characters or less
;;     description -- extended description, string 256 characters or less
;;     link -- URL pointing to location for more information
;; -----------------------------------------------------------------
(define-method asset-type-contract (initialize _environment _name _description _link)
  (assert (equal? creator (send _environment 'get-originator-id)) "only creator may initialize the type")
  (assert (not asset-type-initialized) "asset type already initialized")

  (assert (and (string? _name) (<= (string-length _name) 32)) "invalid name")
  (assert (and (string? _description) (<= (string-length _description) 256)) "invalid description")
  (assert (and (string? _link) (<= (string-length _link) 128)) "invalid link")

  (instance-set! self 'name _name)
  (instance-set! self 'description _description)
  (instance-set! self 'link _link)
  (instance-set! self 'asset-type-initialized #t)

  (dispatch-package::return-success #t))

(define-method asset-type-contract (get-identifier _environment)
  (assert asset-type-initialized "asset type not initialized")
  (dispatch-package::return-value (send _environment 'get-contract-id) #f))

(define-method asset-type-contract (get-name _environment)
  (assert asset-type-initialized "asset type not initialized")
  (dispatch-package::return-value name #f))

(define-method asset-type-contract (get-description _environment)
  (assert asset-type-initialized "asset type not initialized")
  (dispatch-package::return-value description #f))

(define-method asset-type-contract (get-link _environment)
  (assert asset-type-initialized "asset type not initialized")
  (dispatch-package::return-value link #f))

(define-method asset-type-contract (get-creator _environment)
  (assert asset-type-initialized "asset type not initialized")
  (dispatch-package::return-value creator #f))
