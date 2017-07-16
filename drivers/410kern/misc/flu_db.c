/** @file flu_db.h
 *
 *  @brief Database of "Escape the Flu Virus" mazes.
 *  This game is similar to "Theseus and the Minotaur",
 *  invented by Robert Abbott and described in the April
 *  1994 issue of "Games & Puzzles", as documented here:
 *  http://web.archive.org/web/20160711105035/http://www.logicmazes.com/theseus4.html
 *  "Escape the Flu Virus" is not endorsed in any way by
 *  Robert Abbott.
 *
 *  @author Tim Becker (tjbecker)
 *  @author Dave Eckhardt (de0u)
 *  @author Connor Watts (connorwa)
 *  @author Oleg Mazonka
 */

#include <flu_db.h>


const flu_maze_t flu_mazes[] = {
    {
        .origin = "de0u - \"...happily ever after.\"",
        .width = 13,
        .height = 3,
        .maze = {
            "+-+-+-+-+-+-+",
            "|V . . @ . H|",
            "+-+-+-+-+-+-+",
        }
    },
    {
        .origin = "connorwa",
        .width = 17,
        .height = 11,
        .maze = {
            "+-+-+-+-+-+-+-+-+",
            "|. . .|. . . . .|",
            "+     +         +",
            "|. . .|. . . . .|",
            "+     +         +",
            "|. . @ . . . . H|",
            "+     +         +",
            "|. . .|. . . . .|",
            "+     +         +",
            "|V . .|. . . . .|",
            "+-+-+-+-+-+-+-+-+",
        }
    },
    {
        .origin = "connorwa",
        .width = 15,
        .height = 15,
        .maze = {
            "+-+-+-+-+-+-+-+",
            "|@ . . . . . H|",
            "+ +-+ +-+-+   +",
            "|.|. . .|. . .|",
            "+ + +-+ +     +",
            "|.|. . .|. . .|",
            "+ +     +     +",
            "|.|. . .|. . .|",
            "+ +     +     +",
            "|V|. . .|. . .|",
            "+ +     +     +",
            "|.|. . .|. . .|",
            "+ +     +     +",
            "|.|. . . . . .|",
            "+-+-+-+-+-+-+-+",
        }
    },
    {
        .origin = "tjbecker - \"Trap\"",
        .width = 7,
        .height = 7,
        .maze = {
            "+-+-+-+",
            "|. V .|",
            "+ +-+ +",
            "|H|. .|",
            "+ +-+ +",
            "|. @ .|",
            "+-+-+-+",
        }
    },
    {
        .origin = "tjbecker - \"410\"",
        .width = 23,
        .height = 13,
        .maze = {
            "+-+-+-+-+-+-+-+-+-+-+-+",
            "|. . . . . . . . . . .|",
            "+ +-+ +-+ +-+ +-+-+-+ +",
            "|.|.|.|.|.|.|.|. . .|.|",
            "+ + +-+ + + + + +-+ + +",
            "|.|. . .|.|.|.|.|.|.|.|",
            "+ +-+-+ + + + + + + + +",
            "|. . .|.|.|.|.|.|.|.|.|",
            "+     + | + + + +-+ + +",
            "|@ . .|.|.|.|.|. . .|V|",
            "+     +-+ +-+ +-+-+-+ +",
            "|. . . . . H . . . . .|",
            "+-+-+-+-+-+-+-+-+-+-+-+",
        }
    },
    {
	    .origin = "Oleg Mazonka (converted by connorwa)",
    	.width = 11,
    	.height = 9,
    	.maze = {
    		"+-+-+-+-+-+",
    		"|.|. . .|H|",
    		"+ + +-+ + +",
    		"|. @ .|. .|",
    		"+ +   + + +",
    		"|.|. .|.|.|",
    		"+-+-+ + + +",
    		"|. . . .|V|",
    		"+-+-+-+-+-+",
    	}
    },
    {
    	.origin = "Oleg Mazonka (converted by connorwa)",
    	.width = 13,
    	.height = 11,
    	.maze = {
    		"+-+-+-+-+-+-+",
    		"|. . . . . @|",
    		"+-+   + +-+-+",
    		"|. . .|.|. .|",
    		"+-+ + +-+-+ +",
    		"|.|.|. . . .|",
    		"+ +-+ +-+-+ +",
    		"|. . . .|.|.|",
    		"+ +-+-+-+ +-+",
    		"|. . . H V .|",
    		"+-+-+-+-+-+-+",
    	}
    },
    {
	    .origin = "Oleg Mazonka (converted by connorwa)",
	    .width = 13,
	    .height = 13,
	    .maze = {
    	    "+-+-+-+-+-+-+",
    	    "|V . . . . .|",
    	    "+ +-+ +-+   +",
    	    "|. .|. . . .|",
    	    "+ +-+ + +-+ +",
    	    "|H . .|.|. .|",
    	    "+ +-+-+-+   +",
    	    "|. @ .|. . .|",
    	    "+ +-+-+   +-+",
    	    "|. . . . . .|",
    	    "+ +-+ +   +-+",
    	    "|.|. .|. . .|",
    	    "+-+-+-+-+-+-+",
    	}
    },
    {
	    .origin = "Oleg Mazonka (converted by connorwa)",
	    .width = 21,
	    .height = 21,
	    .maze = {
		    "+-+-+-+-+-+-+-+-+-+-+",
		    "|. .|. . . . . . . .|",
		    "+   + +     + +-+ +-+",
		    "|. .|.|. . .|. .|.|.|",
		    "+   +-+-+-+-+   + + +",
		    "|. . .|V .|.|. .|. .|",
		    "+ +   + +-+ +-+-+-+ +",
		    "|H|. .|. . . .|. . .|",
		    "+ +-+ + + +-+ +-+   +",
		    "|.|. . @|. . . . . .|",
		    "+ + +-+ +       +   +",
		    "|.|. . .|. . . .|. .|",
		    "+ +   +-+ + +-+ +   +",
		    "|. . . .|.|. .|. . .|",
		    "+   +-+ + + +-+     +",
		    "|. .|. . .|.|. . . .|",
		    "+ +-+-+-+ + +       +",
		    "|. . . .|. . . . . .|",
		    "+   +-+ +-+-+ + + + +",
		    "|. .|. . . .|.|.|.|.|",
		    "+-+-+-+-+-+-+-+-+-+-+",
    	}
    },
};
    
const int flu_maze_count = sizeof(flu_mazes) / sizeof(flu_mazes[0]);
