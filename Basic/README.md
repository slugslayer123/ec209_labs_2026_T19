![Header Image](https://github.com/ee209-2020class/ee209-2020class.github.io/blob/master/ExtraInfo/logo.png)

# Basic Repository Structure

This is a basic repository structure that is provided for teams that are not confident with Git/GitHub and would like a simpler option than the [advanced structure](../Advanced/).

There are lots of resources provided if you wish to challenge yourself to learn the tools and skills associated with the advanced structure, but we understand that Git/GitHub is not the primary learning outcome of this course.

You may also choose to switch to the advanced structure later on; a TA will be able to help with this.

> [!IMPORTANT]
> - The industry judges will have a look at your repo if you are among the top teams.
> - Familiarity with branches and pull requests is *expected pre-requisite knowledge* in `COMPSYS 302`.
> - EEE students will also continue to use GitHub in `ELECTENG 311`, and possibly in the industry even as a hardware design engineer.


## Usage

- Once a solution for a component related to the project has been developed by a student (in their own folder), and reviewed by the team members, these changes can be migrated to the `FinalImplementation` folder
  - This is to make sure you will not have any merge conflicts (i.e. no one works on the same file at the same time) 
- As a case study, consider one student working on `UART` code while another student is working on `ADC` code
  - This work should be done in your individual folders
  - Once you validate your code individually and all team members are in agreement, the two new features can be transferred to the `FinalImplementation` folder (for example copy-pasting the source file for each module and code section that goes in the `main.c`)
  - You will start realising the benefits of writing modular code


> [!NOTE]
> Though this is not the intended way to use Git and is considered bad practice, it will keep things simple for you until you become more familiar with Git.
