/*****************************************************************************/
/*                                                                           */
/*			  	  lineinfo.h                                 */
/*                                                                           */
/*			Source file line info structure                      */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2001-2011, Ullrich von Bassewitz                                      */
/*                Roemerstrasse 52                                           */
/*                D-70794 Filderstadt                                        */
/* EMail:         uz@cc65.org                                                */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



/* common */
#include "check.h"
#include "lidefs.h"
#include "xmalloc.h"

/* ld65 */
#include "error.h"
#include "fileinfo.h"
#include "fileio.h"
#include "fragment.h"
#include "lineinfo.h"
#include "objdata.h"
#include "segments.h"



/*****************************************************************************/
/*     	       	      	      	     Code			     	     */
/*****************************************************************************/



static LineInfo* NewLineInfo (void)
/* Create and return a new LineInfo struct with mostly empty fields */
{
    /* Allocate memory */
    LineInfo* LI = xmalloc (sizeof (LineInfo));

    /* Initialize the fields */
    LI->File       = 0;
    LI->Type       = LI_TYPE_ASM;
    LI->Pos.Name   = INVALID_STRING_ID;
    LI->Pos.Line   = 0;
    LI->Pos.Col    = 0;
    LI->Fragments  = EmptyCollection;
    LI->Spans      = EmptyCollection;

    /* Return the new struct */
    return LI;
}



void FreeLineInfo (LineInfo* LI)
/* Free a LineInfo structure. This function will not handle a non empty
 * Fragments collection, it can only be used to free incomplete line infos.
 */
{
    unsigned I;

    /* Check, check, ... */
    PRECONDITION (CollCount (&LI->Fragments) == 0);

    /* Free all the code ranges */
    for (I = 0; I < CollCount (&LI->Spans); ++I) {
        FreeSpan (CollAtUnchecked (&LI->Spans, I));
    }

    /* Free the collections */
    DoneCollection (&LI->Spans);

    /* Free the structure itself */
    xfree (LI);
}



LineInfo* GenLineInfo (const FilePos* Pos)
/* Generate a new (internally used) line info with the given information */
{
    /* Create a new LineInfo struct */
    LineInfo* LI = NewLineInfo ();

    /* Initialize the fields in the new LineInfo */
    LI->Pos = *Pos;

    /* Return the struct read */
    return LI;
}



LineInfo* ReadLineInfo (FILE* F, ObjData* O)
/* Read a line info from a file and return it */
{
    /* Create a new LineInfo struct */
    LineInfo* LI = NewLineInfo ();

    /* Read/fill the fields in the new LineInfo */
    LI->Type     = ReadVar (F);
    LI->Pos.Line = ReadVar (F);
    LI->Pos.Col  = ReadVar (F);
    LI->File     = CollAt (&O->Files, ReadVar (F));
    LI->Pos.Name = LI->File->Name;

    /* Return the struct read */
    return LI;
}



void ReadLineInfoList (FILE* F, ObjData* O, Collection* LineInfos)
/* Read a list of line infos stored as a list of indices in the object file,
 * make real line infos from them and place them into the passed collection.
 */
{
    /* Read the number of line info indices that follow */
    unsigned LineInfoCount = ReadVar (F);

    /* Read the line infos and resolve them */
    while (LineInfoCount--) {

        /* Read an index */
        unsigned LineInfoIndex = ReadVar (F);

        /* The line info index was written by the assembler and must
         * therefore be part of the line infos read from the object file.
         */
        if (LineInfoIndex >= CollCount (&O->LineInfos)) {
            Internal ("Invalid line info index %u in module `%s' - max is %u",
                      LineInfoIndex,
                      GetObjFileName (O),
                      CollCount (&O->LineInfos));
        }

        /* Add the line info to the collection */
        CollAppend (LineInfos, CollAt (&O->LineInfos, LineInfoIndex));
    }
}



void RelocLineInfo (Segment* S)
/* Relocate the line info for a segment. */
{
    unsigned long Offs = S->PC;

    /* Loop over all sections in this segment */
    Section* Sec = S->SecRoot;
    while (Sec) {
       	Fragment* Frag;

       	/* Adjust for fill bytes */
       	Offs += Sec->Fill;

       	/* Loop over all fragments in this section */
       	Frag = Sec->FragRoot;
       	while (Frag) {

            unsigned I;

       	    /* Add the range for this fragment to all line infos */
            for (I = 0; I < CollCount (&Frag->LineInfos); ++I) {
                LineInfo* LI = CollAtUnchecked (&Frag->LineInfos, I);
       	       	AddSpan (&LI->Spans, S, Offs, Frag->Size);
       	    }

       	    /* Update the offset */
       	    Offs += Frag->Size;

       	    /* Next fragment */
       	    Frag = Frag->Next;
       	}

       	/* Next section */
       	Sec = Sec->Next;
    }
}




