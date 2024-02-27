{ pkgs ? import <nixpkgs> {}, lib ? pkgs.lib }:

pkgs.stdenv.mkDerivation rec {
  pname = "tinylisp";
  version = "main";

  src = pkgs.fetchFromGitHub {
    owner = "grissess";
    repo = pname;
    rev = "ccbca71ecebcb5d1f10eefcd44a559347fe65d8c";
    hash = "sha256-AXCRMOvnuhviO0T5MqM895JDU8BrBXBA89KXHs9FFys=";
  };

  buildInputs = [ pkgs.gcc pkgs.gnumake ];

  installPhase = ''
    make install DESTDIR=$out
  '';

  meta = {
    description = "A really tiny (<1.5kloc) LISP implementation with metaprogramming (based on the SICP evaluator).";
    homepage = "https://github.com/grissess/tinylisp";
    license = lib.licenses.asl20;
    maintainers = [ { name = "ember arlynx"; email = "ember@lunar.town"; } ];
  };
}
