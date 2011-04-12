/**
   @mainpage Mnemosyne: Lightweight Persistent Memory

   <em> Memoria est thesaurus ominum rerum e custos </em>\n
   Memory is the treasury and guardian of all things. \n
   -- Cicero \n

   <em>A memory is what is left when something happens and does not completely unhappen</em> \n
   -- Edward De Bono \n

   \section section-intro Introduction 

   New storage-class memory (SCM) technologies, such as phase-change memory, 
   STT-RAM, and memristors, promise user-level access to non-volatile storage 
   through regular memory instructions. These memory devices enable fast 
   user-mode access to persistence, allowing regular in-memory data structures 
   to survive system crashes.

   Mnemosyne provides a simple interface for programming with persistent 
   memory. Mnemosyne addresses two challenges: how to create and manage such 
   memory, and how to ensure consistency in the presence of failures. Without 
   additional mechanisms, a system failure may leave data structures in SCM in 
   an invalid state, crashing the program the next time it starts.

   In Mnemosyne, programmers declare global persistent data with the keyword 
   persistent or allocate it dynamically. Mnemosyne provides primitives for 
   directly modifying persistent variables and supports consistent updates 
   through a lightweight transaction mechanism. Compared to past work on 
   disk-based persistent memory, Mnemosyne is much lighter weight, as it can 
   store data items as small as a word rather than a virtual memory page. In 
   tests emulating the performance characteristics of forthcoming SCMs, we 
   find that Mnemosyne provides a 20--280 percent performance increase for 
   small data sizes over alternative persistence strategies, such as 
   Berkeley DB or Boost serialization that are designed for disks.

   \subsection section-intro-name_origin Name Origin

   Mnemosyne is the personification of memory in Greek mythology, and is 
   pronounced nee--moss--see--nee.

   \subsection section-external_references External References

   Haris Volos, Andres Jaan Tack, Michael M. Swift Mnemosyne: Lightweight 
   Persistent Memory. ASPLOS '11: Proceedings of the 16th International 
   Conference on Architectural Support for Programming Languages and 
   Operating Systems, March 2011.

*/

/*****************************************************************************
 *                                GETTING STARTED                            *
 *****************************************************************************/ 

/** 

@page getting_started Getting Started

@section getting_started_building Building

Prerequisites (Linux):
\li Intel C/C++ STM Compiler, Prototype Edition 3.0
\li SCons: A software construction tool

To build xCalls, first check out a copy with SVN. If you have commit access:


\verbatim
% svn co --username username https://xcalls.googlecode.com/svn/trunk $XCALLS
\endverbatim

For anonymous checkout:

\verbatim
% svn co https://xcalls.googlecode.com/svn/trunk $XCALLS
\endverbatim

then open file $XCALLS/SConstruct with your favorite editor and set the environment
variable CC to point to the Intel C Compiler (icc):

\verbatim
env['CC'] = '/path/to/intel/c/compiler/icc'
\endverbatim

and finally build xCalls:

\verbatim
% cd $XCALLS
% scons
\endverbatim

All generated objects and binaries are placed under $XCALLS/build.

You may then install the library into the location of your choice $INSTALL_DIR:

\verbatim
% scons install prefix=$INSTALL_DIR
\endverbatim



 */
