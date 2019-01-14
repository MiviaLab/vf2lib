

all: vflib.html vflib.pdf

vflib.pdf: vflib.sgml
	sgml2latex vflib.sgml
	dvips -o vflib.ps vflib.dvi
	ps2pdf vflib.ps vflib.pdf
	rm vflib.dvi vflib.ps

vflib.html: vflib.sgml
	sgml2html -s 2 vflib.sgml
