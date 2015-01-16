;; Keywords for syntax highlighting

(add-to-list 'auto-mode-alist '("\\.pac2\\'" . binpacpp-mode))

(setq binpacpp-functions '("to_uint" "split1" "mod"))

(setq binpacpp-keywords '("enum" "type" "unit" "var" "module" "export" "self" "switch" "on"))

(setq binpacpp-types '("bytes" "uint8" "uint16" "uint32" "uint64" "&length" "&transient" "&convert" "list"))

(setq binpacpp-constants '("%done" "%init" "0x0"))

(setq binpacpp-functions-regexp (regexp-opt binpacpp-functions 'words))
(setq binpacpp-keywords-regexp (regexp-opt binpacpp-keywords 'words))
(setq binpacpp-types-regexp (regexp-opt binpacpp-types 'words))
(setq binpacpp-constants-regexp (regexp-opt binpacpp-constants 'words))
(setq binpacpp-functions nil)
(setq binpacpp-keywords nil)
(setq binpacpp-types nil)
(setq binpacpp-constants nil)

(setq binpacpp-font-lock-defaults
      `(
        (,binpacpp-constants-regexp . font-lock-constant-face)
        (,binpacpp-types-regexp . font-lock-type-face)
        (,binpacpp-functions-regexp . font-lock-function-name-face)
        (,binpacpp-keywords-regexp . font-lock-keyword-face)
        )
      )

;; Syntax table
(defvar binpacpp-syntax-table nil "Syntax table for `binpacpp-mode'.")
(setq binpacpp-syntax-table
      (let ((synTable (make-syntax-table)))
        ;; bash-style comments
        (modify-syntax-entry ?# "< b" synTable)
        (modify-syntax-entry ?\n "> b" synTable)
        ;;(modify-syntax-entry ?. "w" synTable)
        (modify-syntax-entry ?_ "w" synTable)
        ;;(modify-syntax-entry ?@ "w" synTable)
        synTable))

;; Hooks
(defvar binpacpp-mode-hook nil)
(add-hook 'binpacpp-mode-hook '(lambda ()
     (local-set-key (kbd "RET") 'newline-and-indent)))
(add-hook 'binpacpp-mode-hook '(lambda ()
    (local-set-key (kbd "TAB") 'self-insert-command)))

;; Mode definition
(define-derived-mode binpacpp-mode fundamental-mode
  "binpacpp-mode is a mode for editing BinPAC++ (.pac2) definitions"
  :syntax-table binpacpp-syntax-table

  (setq font-lock-defaults '((binpacpp-font-lock-defaults)))
  (setq mode-name "BinPAC++ definition")

  (setq comment-start "#")
  (setq indent-tabs-mode 1)
  (setq default-tab-width 4)
  (setq indent-line-function 'indent-relative)
  (setq tab-width 4)
  (run-hooks 'binpacpp-mode-hook)
)

(provide 'binpacpp-mode)
