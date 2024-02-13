## Frechet example

#### Datasets
To play with the Frechet application multiple datasets can be used. In the datasets folder are contained two extracts of real datasets called taxi1 and taxi2. Those datasets are meant to test the application from the functionality pint of view. 

To asses performance metrics bigger datasets can be built using the ``generate.py`` script. It can be customized specifying the number of cluster for set S and set R, number of trajectories per cluster and the number of trajectories composing the "noise".

There is also a small script to perform dataset splitting and scattering (see ``split.sh``).
***
#### Execution
	The application takes as arguments:

 1. configuration file for framework
 2. dataset path
 3. number of lines in dataset (chunk) 

##### Single machine 

    make SINGLE_PROCESS=1
    echo "localhost #mapper #reducer" > conf.config
    ./main conf.config ./datasets/taxi2 100 

##### Distributed execution

 - Compile the executable with your compile-time settings, make sure to not compile with SINGLE_PROCESS directive.
 - Write the number of groups you want to deploy in the configuration file each one with the number of actual number of mappers and reducers.
 - Translate/compile the configuration file to obtaine the one used by distributed FastFlow, use the Python script `python ../../util/ff-config-builder.py`.
 - Split the dataset of your choice in n-chunks where n is the same number of groups you want to deploy.
 - Distribute the dataset, the executable and configuration files across the nodes of your choice, if there is no shared filesystem between the nodes (e.g., NFS).
 - Execute the application using the preferred method (manually or using *dff_run* utility)

````
ssh -t compute1 <path_to_executable>/main <path_to_config>/conf.config <path_to_dataset>/taxi2 50 --DFF_Config=<path_to_config>/dff_config.json --DFF_GName=G0
ssh -t compute2 <path_to_executable>/main <path_to_config>/conf.config <path_to_dataset>/taxi2 50 --DFF_Config=<path_to_config>/dff_config.json --DFF_GName=G1
````
or
````
dff_run -V -f <path_to_config>/dff_config.json <path_to_executable>/main <path_to_config>/conf.config <path_to_dataset>/taxi2 50 
````
***
   For support feel free to open an issue.
