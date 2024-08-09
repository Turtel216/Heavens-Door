# Heaven's Door

### A simple terminal based text editor written in the C programming language

> [!NOTE]
> The editor is usable but still under development.

## Development plan

The Current version of Heaven's Door is **0.6**. This is my plan for the feature:

- **Version 0.6** Improve and extend current features. *(DONE)*

- **Version 0.7** Add text search.

- **Version 0.8** Add Syntax highliting for my esoteric programing language [Xanadu](https://github.com/Turtel216/Xanadu).

- **Version 0.9** Oops. Forgot about DO/UNDO.

- **Version 1.0** Code improvements and bug fixing.

## Hot keys so far

> [!NOTE]
> These will change most certainly as vim like motions are added

### Opening a file and closing files

- To open a file add the file path after the executable. For example, after build the executable in ./build you open the test0.txt file in the ./test-files directory like so:
  
         ./build/rohan ./test-files/test.txt

- To create a new file run the executable normally and when you are done press **ctrl+s**. The the editor will ask you for a name and create the file under that name

- To quit the editor press **ctrl+q**

- To save the current file press **ctrl+s**. To force quit the program without saving press **ctrl+q** 3 times

### Moving around

- To move around in all direction use the **arrow-keys**

- To jump to the start/end of a line press the **home**/**end** key respectively

- To jump to the beginning/end of a file press the **page-up**/**page-down** keys respectively

- You can search and jump to a specifc string of characters by pressing **ctrl+f**
### Inserting characters

- Type out the characters that you want and they will be added to the text string. This includes **spaces**

- To add a new line press **enter**.

- To delete backwards use the **backspace** or **ctrl+h** keys. To delete forwards press the **Delete** key.
