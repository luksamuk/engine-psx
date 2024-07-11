((nil
  (eval . (let ((root (projectile-project-root)))
	    (let ((includes (list "/opt/psn00bsdk/include/libpsn00b"
				  (concat root "include")))
		  (neotreebuf (seq-filter (lambda (buf) (equal (buffer-name buf) " *NeoTree*"))
					  (buffer-list))))
	      (setq-local flycheck-clang-include-path includes)
	      (setq-local flycheck-gcc-include-path includes)
	      (when neotreebuf
		(with-current-buffer (first neotreebuf)
		  (setq-local neo-hidden-regexp-list
			      (append '("\\pcsx.json$"
					"\\.frag$"
					"\\.vert$"
					"\\.lua$"
					"\\.mcd$")
				      (default-value 'neo-hidden-regexp-list)))
		  (neotree-refresh))))))))

