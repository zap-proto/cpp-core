;;; zap-mode.el --- Major mode for editing Capn' Proto Files

;; This is free and unencumbered software released into the public domain.

;; Author: Brian Taylor <el.wubo@gmail.com>
;; Version: 1.0.0
;; URL: https://github.com/zap/zap

;;; Commentary:

;; Provides basic syntax highlighting for zap files.
;;
;; To use:
;;
;; Add something like this to your .emacs file:
;;
;; (add-to-list 'load-path "~/src/zap/highlighting/emacs")
;; (require 'zap-mode)
;;

;;; Code:

(defvar zap--syntax-table
  (let ((syn-table (make-syntax-table)))

    ;; bash style comment: “# …”
    (modify-syntax-entry ?# "< b" syn-table)
    (modify-syntax-entry ?\n "> b" syn-table)

    syn-table)
  "Syntax table for `zap-mode'.")

(defvar zap--keywords
  '("struct" "enum" "interface" "union" "import"
    "using" "const" "annotation" "extends" "in"
    "of" "on" "as" "with" "from" "fixed")
  "Keywords in `zap-mode'.")

(defvar zap--types
  '("union" "group" "Void" "Bool" "Int8" "Int16"
    "Int32" "Int64" "UInt8" "UInt16" "UInt32"
    "UInt64" "Float32" "Float64" "Text" "Data"
    "AnyPointer" "AnyStruct" "Capability" "List")
  "Types in `zap-mode'.")

(defvar zap--font-lock-keywords
  `(
    (,(regexp-opt zap--keywords 'words) . font-lock-keyword-face)
    (,(regexp-opt zap--types 'words) . font-lock-type-face)
    ("@\\w+" . font-lock-constant-face))
  "Font lock definitions in `zap-mode'.")

;;;###autoload
(define-derived-mode zap-mode prog-mode
  "capn-mode is a major mode for editing zap protocol files"
  :syntax-table zap--syntax-table

  (setq-local comment-start "# ")
  (setq-local comment-start-skip "#+\\s-*")
  (setq font-lock-defaults '((zap--font-lock-keywords)))
  (setq mode-name "zap"))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.zap\\'" . zap-mode))

(provide 'zap-mode)
;;; zap-mode.el ends here

