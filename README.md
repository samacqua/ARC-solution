This is a fork from top-quark's [top submission](https://github.com/top-quarks/ARC-solution) from the 2020 Abstraction and Reasoning Challenge Kaggle Competition.

This fork will comment the code to make it readable for researchers, and make it easier to build on different machines.

Below you can find a outline of how to run the solution to Kaggle's Abstraction and Reasoning Challenge.

## Tested computer setup
Any Python 3 and g++ supporting c++17 should work (with a little debugging). If you are on a mac, try changing line 7 in the makefile from ```LIBS = -lstdc++ -lstdc++fs``` to ```LIBS = -lstdc++``` (so delete ```lstdc++fs```).

## Running on public data
The comptition data is already in the "dataset" folder for conveniece.

You can run the model using:

```python3 run.py```

You can also add command line arguments:
```--tasks x y z``` will run tasks numbered ```x``` , ```y``` , ```z``` . Defaults to all tasks.
```--train``` controls whether to run the training tasks or validation tasks. Can use 'yes', 'true', 't', 'y', or '1' for True (to run the training set), or 'no', 'false', 'f', 'n', '0' for false (run the validation set). Defaults to training set.
```--depth``` controls the search depth. Takes an integer, defaults to 2.

You can view a summary of the results with:

```python3 summary.py```

Takes 1 argument:
```--train``` controls whether to evaluate as the training tasks or validation tasks. Can use 'yes', 'true', 't', 'y', or '1' for True (to run the training set), or 'no', 'false', 'f', 'n', '0' for false (run the validation set). Defaults to training set.

Running ```python3 run.py``` and ```python3 summary.py``` with default parameters should take about a minute to run and get 180/416.

## Running on test data
To run the full model and produce precictions on the test set (takes 9 hours), change "eval" to 1 on line 75 on src/runner.cpp and run

```python3 safe_run.py```

This produces the output file named "submission_part.csv", which can be renamed to "submission.csv" to submit to the competition.