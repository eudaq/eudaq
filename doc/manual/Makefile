
manual: EUDAQUserManual.gls
	latexmk -pdf EUDAQUserManual.tex

EUDAQUserManual.gls: glossary.tex
	makeglossaries EUDAQUserManual

clean:
	rm -f EUDAQUserManual.pdf *.aux *.log *.toc

.PHONY: manual clean
