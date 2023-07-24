{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/23.05";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      packages.${system} = rec {
        default = sgfs;
        sgfs = pkgs.stdenv.mkDerivation {
          name = "simgridFS";
          version = "0.0";
          src = ./src;
          buildInputs = with pkgs; [
            gnumake
            gnat
            pkg-config 
            fuse3
          ];
          propagatedBuildInputs = [
            pkgs.simgrid
          ];
          buildPhase = ''
            make
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp sgfs $out/bin
          '';
        };
      };

      devShells.${system} = {
        default = pkgs.mkShell {
          buildInputs = with pkgs; [
            simgrid
            gnumake
            gnat
            pkg-config 
            fuse3
            ior
            openmpi
          ];
        };

        report = pkgs.mkShell {
          buildInputs = [
            pkgs.emacs
          ];
          shellHook = ''
            ${pkgs.emacs}/bin/emacs -q -l ./.init.el notes.org
            exit
          '';
        };
      };
    };
}
