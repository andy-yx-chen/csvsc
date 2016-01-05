========================================================================
    CSV Console Editor
========================================================================

This tool provides a easy way for you to play with CSV based data file. It's easy to filter data, add or remove columns, 
save the data back to files with sorting.

Commands supported by this tool,

help
<<shows all the commands>>

columns
<<shows all columns in this CSV file>>

add
values:> value1,value2,value3... (values must match the columns)

delete
what:> [col1] = 'value1' AND [col2] <> 'value2' OR NOT ([col3] = 'value3')

update
set:> [col1]='value1',[col2]='value2',...
for:> [col1] = 'value1' AND [col2] <> 'value2' OR NOT ([col3] = 'value3')

save
file:> save-to-file-path.csv
order:> [col1] [ASC|DESC], [col2] [ASC|DESC],..., example, [col1],[col2],[col3], default to all columns in ASC

exit
<<exit the application>>

batchadd
values:> value-provider1,value-provide2,value-provider3... (value providers must match the columns)
value provider:
    const-provider, const string
    file-provider, @<file-name>
    csv-provider, @<csv-file-name>|column-name
example,
values:> @countries.txt,const-value,@employees.csv/Name

show
columns:> [col1],[col2],[col3],...
query:> [col1] = 'value1' AND [col2] <> 'value2' OR NOT ([col3] = 'value3')
order:> [col1] [ASC|DESC] (or could be empty)

addcolumn
column:> coln
default:> default-value

dropcolumn
column:> col1

reduce
by:> [col1],[col2]
columns:> col1,col2,col3...
values:>[col1],[col2],CAST((CAST([col4] AS REAL) - CAST([col5] AS REAL))/CAST([col4] AS REAL) AS TEXT)
order:> [col1]

vac
values-as-columns, which is turning the values into columns for a csv file, e.g.,
Year,Product,Revenue
2015,A,100000
2015,B,150000
2015,C,100000
2014,A,90000
2014,B,130000
with the following parameters
by:> Year
from:> Product
value:> Revenue
default:> 0
would turn the table into
Year,A,B,C
2015,100000,150000,100000
2014,90000,130000,0
with this change, it would be very easy for you to draw charts in excel