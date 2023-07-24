# SimGridFS

A file-system with FUSE and SimGrid aiming (at term) to simulate PFS locally.

## Install

```bash
nix build
```

## Start

```bash
sgfs <platform> <simgrid options> -- <mount point>
```

! don't forget to unmount
