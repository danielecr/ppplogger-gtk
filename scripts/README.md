# utity scripts

This code was used to periodically parse csv /var/log/messages file and grab the lines related to ppp connection, importing it directly in the database.

Today this kind of staff would be a single line for a tool like fluentd (https://www.fluentd.org/) as input filter, and sql command for the output filter if one want the same effect.


