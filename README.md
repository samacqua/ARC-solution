This is a fork from top-quark's [top submission](https://github.com/top-quarks/ARC-solution) from the 2020 Abstraction and Reasoning Challenge Kaggle Competition.

This fork will comment the code to make it readable for researchers, and make it easier to build on different machines.

Below you can find a outline of how to run the solution to Kaggle's Abstraction and Reasoning Challenge.

## Tested computer setup
Any Python 3 and g++ supporting c++17 should work (with a little debugging).

## Running on public data
The comptition data is already in the "dataset" folder for conveniece.

You can run the model on the evaluation dataset using depth 2 (takes around 1 minute):

```python3 run.py```

You can view a summary of the results with:

```python3 summary.py```

It should give 129 / 419 correct predictions.

To run using depth 3, change "run_depth" to 3 in run.py<br/>
To run on the training dataset, change "bool training = true;" on line 86 in src/runner.cpp, and set "train=True" in summary.py


## Running on test data
To run the full model and produce precictions on the test set (takes 9 hours), change "eval" to 1 on line 75 on src/runner.cpp and run

```python3 safe_run.py```

This produces the output file named "submission_part.csv", which can be renamed to "submission.csv" to submit to the competition.