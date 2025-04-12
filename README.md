A Linux-compatible GTA image CD-ROM (.img) tool to:

* Extract (copy) files from GTA images.

* Replace files in the image with arbitrary files.

## Supported games:

Bully: Scholarship Edition.

Grand Theft Auto: III.

Grand Theft Auto: Vice City.

Grand Theft Auto: San Andreas.

Grand Theft Auto: IV.

## Usage:

```
./gta-img -e <file> <image> # Extracts <file> from <image> to the path that <file> is assigned to.
```

```
./gta-img -r <file|folder> <image> # Replace <file|folder> with equivalent files in the <image>.
```

## Building

```
$ make
```
