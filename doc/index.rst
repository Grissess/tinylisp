.. TinyLisp documentation master file, created by
   sphinx-quickstart on Thu Jul  5 20:34:35 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to TinyLisp!
====================

TinyLisp is a tiny (<1.5kloc of ANSI C99) implementation of LISP, borne from
the traditional eval/apply loop represented in the *Structure and
Interpretation of Computer Programs* (Abelson, Sussman, Sussamn 1996).
Nowadays, due to continuation support, it's difficult to say it is an *exact*
transliteration, but it still resembles its roots.

TinyLisp is quick to build and to use; from the source repository, run `make
run`, optionally setting the environment variable `CC` to your favorite
compiler (or install `gcc` for the default). For more information, see the
`Language Guide <lang>`.

.. toctree::
    :maxdepth: 2
    :caption: Contents:

    src/index
    lang/index


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
