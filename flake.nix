{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-parts.inputs.nixpkgs-lib.follows = "nixpkgs";
    systems.url = "github:nix-systems/default";
  };
  outputs = inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; }
    {
     systems = import inputs.systems;
     perSystem = { self', pkgs, lib, system, ...} :
        {
          packages.default = pkgs.stdenv.mkDerivation
              (finalAttrs: {
                name = "tinylisp";
                # TODO the prebuild is really slow and I'm not sure why :(
                src = ./.;
                installPhase = ''
                  make install DESTDIR=$out
                '';
              });

          devShells.default =
            pkgs.mkShell {
              packages = with pkgs; [ gcc gnumake ];
            };
      };

    };
}
