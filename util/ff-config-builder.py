import csv
import json
import os

DEFAULT_PORT = 8000
 
def csv_to_json(csv_file_path, json_file_path):
    #create a dictionary
    data_dict = {}
    groups = []
 
    with open(os.getcwd()+"/"+csv_file_path, encoding = 'utf-8') as csv_file_handler:
        csv_reader = csv.reader(csv_file_handler, delimiter=' ', quotechar='|')
 
        for i, row in enumerate(csv_reader):
            group_dict = {}
            group_dict["name"] = "G"+str(i)
            group_dict["endpoint"] = row[0] + ":" + str(DEFAULT_PORT)
            groups.append(group_dict)
    
    data_dict["groups"] = groups
 
    with open(json_file_path, 'w', encoding = 'utf-8') as json_file_handler:
        json_file_handler.write(json.dumps(data_dict, indent = 4))


csv_file_path = input('Enter the absolute path of the CSV file: ')
json_file_path = input('Enter the absolute path of the JSON file: ')
 
csv_to_json(csv_file_path, json_file_path)