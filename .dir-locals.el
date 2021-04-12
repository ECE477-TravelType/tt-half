((nil . ((eval . (progn
                   (defvar tt-half-mode-map (make-sparse-keymap)
                     "Keymap while tt-half-mode is active.")

                   (define-minor-mode tt-half-mode
                     "A temporary minor mode to be activated only specific to a buffer."
                     nil
                     :lighter " [tt-half]"
                     tt-half-mode-map)

                   (tt-half-mode 1)

                   (defun oats/tt-half-run ()
                     (interactive)
                     (let* ((mk-dir (locate-dominating-file (buffer-file-name) "Makefile"))
                            (compile-command (concat "make -k -C " (shell-quote-argument mk-dir) " run"))
                            (compilation-read-command nil))
                       (call-interactively 'compile)))

                   (defun oats/tt-half-gdb ()
                     (interactive)
                     (gdb (string-join `("arm-none-eabi-gdb -i=mi"
                                         "-cd" ,(locate-dominating-file (buffer-file-name) "gdbinit")
                                         "-x ./gdbinit"
                                         "./build/tt-half.elf")
                                       " ")))

                   (define-key tt-half-mode-map
                     (kbd "C-c C-g") 'oats/tt-half-gdb)
                   (define-key tt-half-mode-map
                     (kbd "C-c C-r") 'oats/tt-half-run))))))
