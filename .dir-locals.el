((nil
  (eval . (let ((root (projectile-project-root)))
	    (let ((includes (list "/opt/psn00bsdk/include/libpsn00b"
				  (concat root "include")))
		  (neotreebuf (seq-filter (lambda (buf) (equal (buffer-name buf) " *NeoTree*"))
					  (buffer-list))))
	      (setq-local flycheck-clang-include-path includes)
	      (setq-local flycheck-gcc-include-path includes)	      
	      ;; (when neotreebuf
	      ;; 	(with-current-buffer (first neotreebuf)
	      ;; 	  (let ((excluded '("\\pcsx.json$"
	      ;; 			    "\\.frag$"
	      ;; 			    "\\.vert$"
	      ;; 			    "\\.lua$"
	      ;; 			    "\\.mcd$")))
	      ;; 	    (unless (every (lambda (n) (not (null n)))
	      ;; 			   (mapcar (lambda (x) (member x neo-hidden-regexp-list))
	      ;; 				   excluded))
	      ;; 	      (setq neo-hidden-regexp-list
	      ;; 		    (append excluded (default-value 'neo-hidden-regexp-list)))
	      ;; 	      (neotree-refresh)))))
	      )))))


