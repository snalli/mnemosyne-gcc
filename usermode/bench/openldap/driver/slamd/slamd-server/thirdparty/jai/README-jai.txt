JavaTM Advanced Imaging API v1.1.2_01 Readme

Contents

   * Introduction
        o Installation and System Requirements
        o Documentation
        o Bugs
   * Changes From JAI 1.1.2 to JAI 1.1.2_01
        o Bugs Fixed in JAI 1.1.2_01
        o Enhancements Added in JAI 1.1.2_01
        o Unresolved Issues in JAI 1.1.2_01
   * Overview of JAI Functionality
        o Core Functionality
        o Operators
             + Native Acceleration
        o How to Run the JAI 1.1 (and later) version of Remote Imaging
        o How to Run the JAI 1.0.2 version of Remote Imaging

   * Feedback

Introduction

The current release, JAI 1.1.2_01, is a patch release intended primarily to
fix a problem in the Windows installers which was observed with pre-FCS
versions of the J2SE 1.5 platform. See "Changes From JAI 1.1.2 to JAI
1.1.2_01" for a listing of the changes in the present patch release.

The Maintenance Review release of the 1.1 version of the Java TM Advanced
Imaging (JAI) specification contains some changes since the JAI 1.1
specification Final Release. The present reference implementation, called
Java Advanced Imaging 1.1.2_01, implements the Maintenance Review version of
the JAI 1.1 specification.

The Java Advanced Imaging API home page is located at
http://java.sun.com/products/java-media/jai/. There you will find binaries,
documentation, answers to frequently asked questions, and other information.

Installation and System Requirements

For general installation instructions please refer to the INSTALL page. For
system requirements please refer to the system requirements section of the
INSTALL page.

Documentation

Links to Java Advanced Imaging documentation are available at
http://java.sun.com/products/java-media/jai/docs/

Bugs

Some bugs are known to exist - see the BUGS page for details.

Changes From JAI 1.1.2 to JAI 1.1.2_01

Bugs Fixed in JAI 1.1.2_01

 Bug ID                               Synopsis
 4990502JAI FAQ is not current (for browsers)

 5044839JAI installation program does not seem to install on Java 1.5 beta
        on Win32

Enhancements Added in JAI 1.1.2_01

 Bug ID                               Synopsis

 4896805JAI FAQ should have an item about file locking in
        FileLoad/FileStore ops

 5090980JAI INSTALL should contain clear instructions on using auto-install
        in browsers

Unresolved Issues in JAI 1.1.2_01

A list of unresolved issues in JAI 1.1.2_01 as of the writing of this
document may be obtained by visiting the JAI bugs page. Up to date
information may be obtained by searching the Bug Database on Category "JAI
is Advanced Imaging" and State "Open". To use the Bug Database you need to
be registered with the Java Developer Connection.

Overview of JAI Functionality

Core Functionality

All areas of the JAI specification have been implemented for this release.
The com.sun.media.jai.codec package continues to be an uncommitted part of
JAI. For mode information on image reading and writing images on the Java 2
platform and in JAI please refer to the page Image I/O in Java Advanced
Imaging.

All operators outlined in the Java Advanced Imaging API specification are
implemented.

The major areas of JAI functionality are described below:

  1. Tiling

     Images are made up of tiles. Different images may have different tile
     sizes. Each tile can be processed and stored in memory separately.
     Tiles may be stored in a centrally-maintained cache for performance.
     The use of tiling also facilitates the use of multiple threads for
     computation. Previously allocated tiles may also be re-used to save
     memory.

  2. Deferred execution

     Image operations performed on an image do not take place immediately
     when they are defined. Calculation of pixels happens only when a
     portion of the resultant image is requested, and only tiles that are
     needed are processed. However, operations always appear semantically to
     be completed immediately.

  3. Threaded Computation

     Requests for tiles are given to several running threads, allowing
     potential speedups on multi-processor systems or when requesting data
     over the network.

  4. Object-Oriented Extensibility

     Users can add their own image operators or override existing operators
     by registering the new operators with the operation registry. Please
     see the "The Java Advanced Imaging API White Paper," the API
     documentation, the JAI tutorial, the sample code, and the jai-interest
     archives for more information (in the archives search for subjects
     beginning with "sample code").

     Additionally, users may extend a number of non-image classes in order
     to add functionality to JAI:

        o Border Extension

          Operators may obtain an extended view of their sources using an
          extensible mechanism for generating pixel values outside of the
          source image bounds. New subclasses of BorderExtender may be used
          to obtain customized functionality.

        o Image Warping

          Subclasses of Warp may be written to perform customized image
          warping.

        o Pixel Interpolation

          Subclasses of Interpolation may be written to perform customized
          pixel interpolation.

        o Color Spaces

          Subclasses of ColorSpaceJAI may be written to implement
          mathematically defined color space transformations without the
          need for ICC profiles.

  5. Graphics2D-Style Drawing

     Graphics2D-style drawing may be performed on a TiledImage in a manner
     analogous to that available for java.awt.image.BufferedImage.

     The RenderableGraphics class provides a way to store a sequence of
     drawing commands and to "replay" them at an arbitrary output
     resolution.

  6. Regions of interest (ROIs)

     Non-rectangular portions of an image may be specified using a ROI or
     ROIShape object. These ROIs may be used as parameters to the Extrema,
     Mean, Histogram or Mosaic operations and the TiledImage.set() and
     TiledImage.setData() methods. Operations produce an appropriate ROI
     property on their output when one exists on their input.

  7. Image file handling

     This release of Java Advanced Imaging supports BMP, FlashPIX, GIF,
     JPEG, PNG, PNM, and TIFF images as defined in the TIFF 6.0
     specification, and WBMP type 0 B/W uncompressed bitmaps. TIFF G3 (1D
     and 2D), G4, PackBits, LZW, JPEG, and DEFLATE (Zip) compression types
     are understood.

     The classes dealing with image file handling (the
     com.sun.media.jai.codec package and private implementation packages
     that provide support for it) are provided in a separate jai file,
     jai_codec.jar. This jar file may be used separately from the
     jai_core.jar file containing the various javax.media.jai packages and
     their supporting classes.

     As described in the Core Functionality section of this document, the
     image codec classes should be considered as temporary helper functions.
     They will be replaced by a new API for image I/O that has been defined
     under the Java Community Process.

        o BMP File Handling:

               The BMP reader can read Version 2.x, 3.x and some 4.x BMP
               images. BMP images with 1, 4, 8, 24 bits can be read with
               this reader. Support for 16 and 32 bit images has also been
               implemented, although such images are not very common.

               Reading of compressed BMPs is supported. BI_RGB, BI_RLE8,
               BI_RLE4 and BI_BITFIELDS compressions are handled.

               The BMP reader emits properties such as type of compression,
               bits per pixel etc. Use the getPropertyNames() method to get
               the names of all the properties emitted.

               BMP Limitations:

                  + Only the default RGB color space is supported.
                  + Alpha channels are not supported.

               BMP Writer:
                  + The BMP writer is capable of writing images in the
                    Version 3 format. Images which make use of a
                    IndexColorModel with 2, 16, or 256 palette entries will
                    be written in palette form.
                  + RLE4 and RLE8 compression is supported when compatible
                    with the image data.

        o FlashPIX file handling:

               A limited FlashPIX reader is provided that is capable of
               extracting a single resolution from a FlashPIX image file.
               Only simple FlashPix files are decoded properly.

               The image view object is ignored, and image property
               information is not exported.

               There is no FlashPIX writer.

        o GIF file handling:

               There is no GIF writer due to the patent on the LZW
               compression algorithm.

        o JPEG file handling:

             + JPEG files are read and written using the classes found in
               the com.sun.image.codec.jpeg package of the JDK. A set of
               simple JAI wrapper classes around these classes is provided.
             + The JPEG decoder cannot read abbreviated streams, either
               tables-only or image-only.

        o PNG file handling:

               All files in the PNGSuite test suite have been read and
               written successfully. See the documentation in
               com.sun.media.jai.codec.PNGDecodeParam and PNGEncodeParam for
               more information.

        o PNM file handling:

               PNM files may be read and written in both ASCII and raw
               formats. The encoder automatically selects between PBM
               (bitmap), PGM (grayscale) and PPM (RGB) files according to
               the number of bands and bit depth of the source image.

               Due to the limitations of the format, only images with 1 or 3
               bands may be written.

        o TIFF file handling:

          TIFF support has the following limitations:

             + The TIFF encoder does not support LZW compression due to the
               patent on the algorithm.
             + Planar format (PlanarConfiguration field has value 2) is not
               supported for decoding or encoding.

        o WBMP file handling:

               The WBMP codec reads and writes images in the Wireless Bitmap
               format described in chapter 6 and Appendix A of the Wireless
               Application Protocol (WAP) Wireless Application Environment
               Specification, Version 1.3, 29 March 2000. The WBMP type
               supported is WBMP Type 0: B/W, Uncompressed Bitmap. There are
               no limitations on the image dimensions.

  8. Image Layouts

     Images with arbitrary pixel layouts may be processed in a uniform
     manner using the PixelAccessor and RasterAccessor classes.

     Source images with ComponentColorModels and IndexColorModels are
     supported. DirectColorModel images are not supported.

     PixelAccessor and RasterAccessor provide the most efficient support for
     the ComponentSampleModel/ComponentColorModel combination.

  9. Image Collections

     The output of a standard image operator on an instance of
     java.util.Collection is a collection of the same type. Nested
     collections are supported. Operators may also emit collections of their
     choice, or take collections as sources and emit a single image.

 10. Remote Imaging

     JAI allows operations to be performed remotely to be created in a
     manner similar to local operations. RemoteJAI.create() and
     RemoteJAI.createRenderable() can be used to create operations that are
     performed on remote hosts. Operation chains are created on the client
     and can contain a mix of local and remote operations by using
     JAI.create() and RemoteJAI.create(), respectively to create the
     operations.

     The "fileload" and "filestore" operations can allow files that reside
     only on remote filesystems to be loaded and stored remotely. This can
     be accomplished by setting the checkFileLocally argument to the
     operation to be false, in which case the presence of the file to be
     loaded or stored is not checked on the local file system when the
     operation is first created.

     See sections below for instructions on how to use the JAI 1.0.2 and 1.1
     or later versions of remote imaging.

 11. Iterators

     Optimized Rect and Random iterators exist as well as a non-optimized
     version of the Rook iterator.

 12. Snapshotting of External Sources

     SnapshotImage provides an arbitrary number of synchronous views of a
     possibly changing WritableRenderedImage.

 13. Meta-data Handling

     Meta-data handling is provided via a name-value database of properties
     associated with each JAI image. Mechanisms are provided by which such
     properties may be generated and processed in the course of image
     manipulation. The ability to defer computation of such data is also
     provided in a manner conceptually equivalent to that available for
     image data. Please refer to the DeferredData and DeferredProperty
     classes for more information.

 14. Serialization Support

     SerializerFactory provides a framework is provided to assist in
     serializing instances of classes which do not implement
     java.io.Serializable. Such objects must be serialized by extracting a
     serializable version of their state from which the original object may
     be extracted after deserialization.

Operators

Java Advanced Imaging extends the imaging functionality provided in the Java
2D API by providing a more flexible and scalable architecture targeted for
complex, high performance imaging requirements. In this context a large
number of imaging operators are provided.

   * Native Acceleration

     Pure Java implementations are provided for all image operators and
     imaging performance is addressed for some of these by providing C-based
     native code. Native C-code based acceleration for a certain subset of
     operators under specific conditions (listed in the table below) is
     available for the Sun/Solaris, Win32 and Linux (x86 only) platforms. On
     Sun UltraSPARC-based platforms, additional performance is gained with
     hardware acceleration via the VIS instructions for most natively
     supported operators. On Win32 platforms which support MMX instructions,
     hardware acceleration is gained for a subset of the natively supported
     operators.

     If a native implementation is present it is, by default, the preferred
     implementation. But if the nature of the sources and parameters of the
     operation are incompatible with the native operation then processing
     will revert to Java code. In general the following minimum requirements
     must be adhered to for the mediaLib native implementation of an
     operation to be executed:

        o All sources must be RenderedImages.
        o All sources and destination must have
             + a SampleModel which is a ComponentSampleModel and
             + a ColorModel which is a ComponentColorModel or no ColorModel
               (i.e., it is null).
        o All sources and the destination must have at most 4 bands of pixel
          data.
        o If Interpolation type is one of the arguments to the operator,
          then native acceleration is available only for Nearest, Bilinear,
          Bicubic and Bicubic2 cases. Additionally for byte images
          InterpolationTable is also supported for native acceleration.

     Further restrictions may be imposed by individual operations but the
     above are the most common requirements.

   * Imaging Operators

     The following image operators are implemented in this release. Only a
     brief description of each operator is provided here. For detailed
     information on these operators, refer to the package
     javax.media.jai.operator in the full documentation available at
     http://java.sun.com/products/java-media/jai/docs/index.html.

     All operations are performed on a per-band basis, unless specified
     otherwise. C acceleration applies to all platforms whereas VIS is
     specific to Sun UltraSPARC and MMX to Windows.

       1. Point and Arithmetic Operators

                                                     Native Acceleration
              Operator Name       Description
                                                 C VIS MMX      Notes
                                Computes the
           absolute             absolute value   X  X   X
                                of the pixels of
                                an image.
                                Adds the pixel
           add                  values of two    X  X   X
                                source images.
                                Adds a
           addcollection        collection of
                                images to one
                                another.
                                Adds a set of
                                constant values
           addconst             to the pixel     X  X   X
                                values of a
                                source image.
                                Adds a set of
                                constant values
           addconsttocollection to the pixel     X  X   X
                                values of a
                                Collection of
                                source images.
                                And's the pixel
           and                  values of two    X  X   X
                                source images.
                                And's the pixel
                                values of a
           andconst             source image     X  X   X
                                with a set of
                                constants.
                                Computes a
                                linear
           bandcombine          combination of   X  X   X  3x4 matrix only.
                                the bands of an
                                image.
                                Creates an image
                                consisting of
                                all bands of all
           bandmerge            sources
                                concatenated in
                                the order
                                encountered.
                                Selects a subset
                                of the bands of            Only if the band
           bandselect           an image,        X  X   X  selection is
                                possibly                   monotonically
                                reordering them.           increasing.
                                Thresholds a
                                single-band
           binarize             image to two     X  X
                                levels to
                                generate a
                                bilevel output.
                                Set all pixel
                                values below the
                                low value to
                                that low value,
           clamp                set all the      X  X   X
                                pixel values
                                above the high
                                value to that
                                high value.
                                Converts an
           colorconvert         image to a given
                                ColorSpace.
                                Generates an
                                optimal LUT by
           colorquantizer       executing a
                                color
                                quantization
                                algorithm
                                Combines two
                                images based on
           composite            their alpha      X  X   X
                                values at each
                                pixel.
                                Creates an image
           constant             with constant
                                pixel values.
                                Divides the
                                pixel values of
                                the first source
           divide               image by the     X
                                pixel values of
                                the second
                                source image.
                                Divides the
                                pixel values of
           dividebyconst        a source image   X
                                by a set of
                                constants.
                                Divides a set of
           divideintoconst      constants by the X
                                pixel values of
                                a source image.
                                Computes the
           exp                  exponential of   X
                                the pixel values
                                of an image.
                                Performs
                                reformatting on
                                an image,
                                including data
           format               type casting,
                                replacing the
                                SampleModel and
                                ColorModel, and
                                restructuring
                                the tile grid.
                                Inverts the
           invert               pixel values of  X  X   X
                                an image.
                                Computes the
                                natural
           log                  logarithm of the X
                                pixel values of
                                an image.

                                Performs general           Only if table
           lookup               table lookup on  X  X   X  has less than or
                                an image.                  equal to 4
                                                           bands.
                                Performs a
                                piecewise linear
                                remapping of
           matchcdf             pixel values to
                                match a given
                                cumulative
                                distribution
                                function.
                                Chooses the
           max                  maximum pixel    X  X   X
                                values between
                                two images.
                                Chooses the
           min                  minimum pixel    X  X   X
                                values between
                                two images.
                                Multiplies the
           multiply             pixel values of  X  X
                                two source
                                images.
                                Multiplies the
                                pixel values of
           multiplyconst        a source image   X  X
                                by a set of
                                constants.
                                Inverts the
           not                  pixel values of  X  X   X
                                a source image.
                                Or's the pixel
           or                   values of two    X  X   X
                                source images.
                                Or's the pixel
                                values of a
           orconst              source image     X  X   X
                                with a set of
                                constants.
                                Overlays one
           overlay              image on top of
                                another image.
                                Performs
                                piecewise linear
           piecewise            remapping of the
                                pixel values of
                                an image.
                                Performs a
                                linear remapping
           rescale              of the pixel     X  X
                                values of an
                                image.
                                Subtracts the
                                pixel values of
           subtract             one image from   X  X   X
                                those of
                                another.
                                Subtracts a set
                                of constant
           subtractconst        values from the  X  X   X
                                pixel values of
                                an image.
                                Subtracts a set
                                of constant
           subtractfromconst    values from the  X  X   X
                                pixel values of
                                an image.
                                Maps the pixel
                                values that fall
           threshold            between a low    X  X   X
                                and high value
                                to a set of
                                constants.
                                Xor's the pixel
           xor                  values of two    X  X   X
                                source images.
                                Xor's a source
           xorconst             image with a set X  X   X
                                of constants.

       2. Area and Geometric Operators

                                                      Native Acceleration
               Operator Name        Description
                                                  C VIS MMX       Notes

                                 Performs first             InterpolationTable
           affine                order geometric  X  X   X  is not MMX
                                 image warping.             accelerated for
                                                            even byte images.

           border                Adds a border
                                 around an image.
                                 Convolves an
           boxfilter             image using a
                                 two-dimensional
                                 box filter.
                                 Performs an MxN
           convolve              image            X  X   X  General and
                                 convolution.               separable cases.
                                 Extracts a
           crop                  subarea of an
                                 image.
                                 Performs
                                                            Only single band,
           dilate                morphological    X  X   X  3x3 kernels
                                 dilation on an
                                 image.                     centered at 1,1
                                 Performs
                                                            Only single band,
           erode                 morphological    X  X   X  3x3 kernels
                                 erosion on an
                                 image.                     centered at 1,1
                                 Performs a
                                 combined integral
           filteredsubsample     subsample and    X  X   X
                                 symmetric
                                 product-separable
                                 filter.
                                 Performs edge
           gradientmagnitude     detection using  X  X   X
                                 orthogonal
                                 gradient masks.
                                 Computes the               Only single band;
           maxfilter             maximum value of X  X   X  only for a SQUARE
                                 a pixel                    mask of size 3, 5,
                                 neighborhood.              or 7
                                 Computes the
           medianfilter          median value of aX  X   X
                                 pixel
                                 neighborhood.
                                 Computes the               Only single band;
           minfilter             minimum value of X  X   X  only for a SQUARE
                                 a pixel                    mask of size 3, 5,
                                 neighborhood.              or 7
                                 Creates a mosaic
           mosaic                of two or more   X  X   X
                                 rendered images.

                                 Rotates an image           InterpolationTable
           rotate                about an         X  X   X  is not MMX
                                 arbitrary point.           accelerated for
                                                            even byte images.

                                 Scales and                 InterpolationTable
           scale                 translates an    X  X   X  is not MMX
                                 image.                     accelerated for
                                                            even byte images.
                                                            InterpolationTable
           shear                 Shears an image. X  X   X  is not MMX
                                                            accelerated for
                                                            even byte images.
                                 Subsamples an
           subsampleaverage      image by         X  X
                                 averaging over a
                                 moving window.
                                 Subsamples a
           subsamplebinarytogray bilevel image to X  X
                                 a grayscale
                                 image.
                                 Translates an
                                 image by an                InterpolationTable
           translate             integral or      X  X   X  is not MMX
                                 fractional                 accelerated for
                                 amount.                    even byte images.
                                 Reflects an image
                                 in a specified
           transpose             direction or     X  X   X
                                 rotates an image
                                 in multiples of
                                 90 degrees.
                                 Sharpens an image
           unsharpmask           by suppressing   X  X   X  General and
                                 the low                    separable kernels.
                                 frequencies.
                                 Performs
           warp                  geometric warpingX  X   X  polynomial and
                                 on an image.               grid only.

       3. Frequency-domain, Transform, and Complex Operators

                                                               Native
            Operator Name           Description             Acceleration
                                                          C  VIS  MMX Notes

           conjugate        Computes the complex
                            conjugate of an image.

           dct              Computes the Discrete Cosine  X
                            Transform of an image.
                            Computes the Discrete
           dft              Fourier Transform of an       X
                            image, possibly resulting in
                            a complex image.

           dividecomplex    Computes the quotient of two
                            complex images.
                            Computes the inverse
           idct             Discrete Cosine Transform of  X
                            an image.
                            Computes the inverse
           idft             Discrete Fourier Transform    X
                            of an image.

           magnitude        Computes the magnitude of a
                            complex image.
                            Computes the squared
           magnitudesquared magnitude of a complex
                            image.

           multiplycomplex  Computes the product of two
                            complex images.

           periodicshift    Shifts an image
                            periodically.

           phase            Computes the phase angle of
                            a complex image.
                            Creates a complex image from
           polartocomplex   two images representing
                            magnitude and phase.

       4. Statistical Operators

                                              Native
            Operator                       Acceleration
              Name        Description
                                         C  VIS MMX  Notes
                      Computes the
                      maximum and                         Only if the ROI
           extrema    minimum pixel      X   X   X        is null or
                      values of an                        encloses the
                      image.                              entire image.
                      Computes the
           histogram  histogram of an    X   X   X
                      image.
                                                          Only if the ROI
                      Computes the mean                   is null or
           mean       pixel value of a   X   X   X        encloses the
                      region of an                        entire image and
                      image.                              the sampling
                                                          period is 1.

       5. Sourceless Operators

           Operator Name                    Description
           imagefunction Creates an image by evaluating a function.
           pattern       Creates an image consisting of a repeated pattern.

       6. File and Stream Operators

           Operator Name                    Description
           awtimage      Converts a java.awt.Image into a PlanarImage.
           bmp           Loads an image in BMP format.
           encode        Writes an image to an OutputStream.
           fileload      Loads an image from a file.
           filestore     Writes an image to a file in a given format.
           fpx           Loads an image in FlashPIX format.
           gif           Loads an image in GIF format.
                         Reads an image from a remote IIP server,
           iip           performing IIP view transforms (affine,
                         colortwist, filter, crop).

           iipresolution Reads a single resolution of an image from a
                         remote IIP server.
           jpeg          Loads an image in JPEG format.
           png           Loads an image in PNG 1.0 or 1.1 format.
           pnm           Loads an image in PBM, PGM, or PPM format.
           stream        Loads an image from a stream.
           tiff          Loads an image in TIFF 6.0 format.
           url           Loads an image from a URL.

       7. Other Operators

           Operator Name                     Description

           errordiffusion Performs error diffusion color quantization using
                          a specified color map and error filter.
                          Performs no processing. Useful as a placeholder
           null           in an operation chain or as a node which emits
                          meta-data.

           ordereddither  Performs color quantization using a specified
                          color map and a fixed dither mask.

           renderable     Constructs a RenderableImage from a RenderedImage
                          source.

How to Run the JAI 1.1 (and later) version of Remote Imaging

1. Create a Security Policy File

If $JAI is the base directory where Java Advanced Imaging is installed,
create a text file named $JAI/policy containing the following:

  grant {
    // Allow everything for now
    permission java.security.AllPermission;
  };

Note that this policy file is for testing purposes only, and it is not
recommended that full permission be given to all programs.

For more information on policy files and permissions please see:

http://java.sun.com/products/jdk/1.3/docs/guide/security/PolicyFiles.html

http://java.sun.com/products/jdk/1.3/docs/guide/security/permissions.html

2. Start the RMI Registry

Log in to the remote machine where the image server will be running and
start the RMI registry. For example, in the Solaris operating environment
using a Bourne-compatible shell (e.g., /bin/sh):

  $ unset CLASSPATH
  $ rmiregistry &

Note that the CLASSPATH environment variable is deliberately not set.

3. Start the JAI Remote Image Server

While still logged in to the remote server machine, set the CLASSPATH and
LD_LIBRARY_PATH environment variables as required for JAI (see the INSTALL
file) and start the remote imaging server:

  $ CLASSPATH=$JAI/lib/jai_core.jar:$JAI/lib/jai_codec.jar:\
              $JAI/lib/mlibwrapper_jai.jar
  $ export CLASSPATH
  $ LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$JAI/lib
  $ export LD_LIBRARY_PATH
  $ java \
  -Djava.rmi.server.codebase=\
  "file:$JAI/lib/jai_core.jar file:$JAI/lib/jai_codec.jar" \
  -Djava.rmi.server.useCodebaseOnly=false \
  -Djava.security.policy=file:$JAI/policy \
  com.sun.media.jai.rmi.JAIRMIImageServer

For example, when the above steps are executed on a machine with IP address
123.456.78.90 the following is printed:

  Server: using host 123.456.78.90 port 1099
  Registering image server as "rmi://123.456.78.90:1099/JAIRMIRemoteServer1.1".
  Server: Bound RemoteImageServer into the registry.

4. Run the Local Application

Run the local application making sure that the serverName parameter of any
javax.media.jai.remote.RemoteJAI constructors corresponds to the machine on
which the remote image server is running. For example, if the machine with
IP address 123.456.78.90 above is named myserver the serverName parameter of
any RemoteJAI constructors should be "myserver".

How to Run the JAI 1.0.2 version of Remote Imaging

For more information on RMI (remote method invocation) please refer to:
http://java.sun.com/products/jdk/rmi/index.html

1. Create a Security Policy File

If $JAI is the base directory where Java Advanced Imaging is installed,
create a text file named $JAI/policy containing the following:

grant {
  // Allow everything for now
  permission java.security.AllPermission;
};

Note that this policy file is for testing purposes only.

For more information on policy files and permissions please see:

http://java.sun.com/products/jdk/1.2/docs/guide/security/PolicyFiles.html

http://java.sun.com/products/jdk/1.2/docs/guide/security/permissions.html

2. Start the RMI Registry

Log in to the remote machine where the image server will be running and
start the RMI registry. For example, in the Solaris operating environment
using a Bourne-compatible shell (e.g., /bin/sh):

$ unset CLASSPATH
$ rmiregistry &

Note that the CLASSPATH environment variable is deliberately not set.

3. Start the JAI Remote Image Server

While still logged in to the remote server machine, set the CLASSPATH and
LD_LIBRARY_PATH environment variables as required for JAI (see the INSTALL
file) and start the remote imaging server:

$ CLASSPATH=$JAI/lib/jai_core.jar:$JAI/lib/jai_codec.jar:\
            $JAI/lib/mlibwrapper_jai.jar
$ export CLASSPATH
$ LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$JAI/lib
$ export LD_LIBRARY_PATH
$ java \
-Djava.rmi.server.codebase=\
"file:$JAI/lib/jai_core.jar file:$JAI/lib/jai_codec.jar" \
-Djava.rmi.server.useCodebaseOnly=false \
-Djava.security.policy=file:$JAI/policy \
com.sun.media.jai.rmi.RMIImageImpl

For example, when the above steps are executed on a machine with IP address
123.456.78.90 the following is printed:

Server: using host 123.456.78.90 port 1099
Registering image server as
  "rmi://123.456.78.90:1099/RemoteImageServer".
Server: Bound RemoteImageServer into
   the registry.

4. Run the Local Application

Run the local application making sure that the serverName parameter of any
RemoteImage constructors corresponds to the machine on which the remote
image server is running. For example, if the machine with IP address
123.456.78.90 above is named myserver the serverName parameter of any
RemoteImage() constructors should be "myserver".

Copyright 2004 Sun Microsystems, Inc. All rights reserved. Use is subject to
license terms. Third-party software, including font technology, is
copyrighted and licensed from Sun suppliers. Portions may be derived from
Berkeley BSD systems, licensed from U. of CA. Sun, Sun Microsystems, the Sun
logo, Java, and Solaris are trademarks or registered trademarks of Sun
Microsystems, Inc. in the U.S. and other countries. Federal Acquisitions:
Commercial Software - Government Users Subject to Standard License Terms and
Conditions.

Copyright 2004 Sun Microsystems, Inc. Tous droits réservés. Distribué par
des licences qui en restreignent l'utilisation. Le logiciel détenu par des
tiers, et qui comprend la technologie relative aux polices de caractères,
est protégé par un copyright et licencié par des fournisseurs de Sun. Des
parties de ce produit pourront être dérivées des systèmes Berkeley BSD
licenciés par l'Université de Californie. Sun, Sun Microsystems, le logo
Sun, Java, et Solaris sont des marques de fabrique ou des marques déposées
de Sun Microsystems, Inc. aux Etats-Unis et dans d'autres pays.
