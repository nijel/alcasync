#alcatest commands definition file
#
#format:
#
#"name"         "description"           "hex data + %0-%9 as params"    read_packets    param_count    {type    "description"   min_len max_len} ... 
# types:
# S ... string
# X ... hexadecimal
# D ... dword
# W ... word
# B ... byte
#
# min_len and max_len only for S and X types
    

"attach"        "attach to mobile"      "00007c20"                      1               0
"detach"        "detach from mobile"    "00017c00"                      1               0
"start"      "starts session"        "00047c8012345678"              1               0
"start_db"   "starts session"        "00047c80%0"                    1               1               {X      "DBID"          4   4}
"close"      "closes session"        "0004%02301"                    1               1   {B  "type (100=cal, 104=todo, 108=cont)"}
"select_type1"  "select sync type (step 1)" "0000%020"  1   1   {B  "type (100=cal, 104=todo, 108=cont)"}
"select_type2"  "select sync type (step 2)" "0004%0220100"  2   1   {B  "type (100=cal, 104=todo, 108=cont)"}
"begin_transfer"    "starts transfer"   "00047c81%0008500"  2   1   {B  "type (0=cal, 1=cont, 2=todo)" }

"item_ids"  "gets ids of selected type"     "0004%02f01"    2   1   {B  "type (100=cal, 104=todo, 108=cont)"}
"item_fields"    "gets list of fields"      "0004%03001%1"  2   2   {B  "type (100=cal, 104=todo, 108=cont)"}   {D  "item id"}
"item_get_field" "gets field value"         "0004%01f01%1%2"    2   3   {B  "type (100=cal, 104=todo, 108=cont)"}   {D  "item id"}  {B "field"}
"item_update_field" "updates field"         "0004%02601%165%2%337%4" 1 5 {B "type"} {D "item"} {B "length of remain"} {B "field"} {X "data" 1 100}
"item_create_field" "creates field"         "0004%0250165%1%237%2" 1 4 {B "type"} {B "length of remain"} {B "field"} {X "data" 1 100}

"list_ids"  "gets ids of selected list"     "0004%00b%1"    2   2   {B  "type (100=cal, 104=todo, 108=cont)"}   {B "list (155=todo, 150=cont)"}
"list_item" "gets text of selected item"    "0004%00c%10a01%2" 2 3 {B  "type (100=cal, 104=todo, 108=cont)"}   {B "list (155=todo, 150=cont)"}  {B "item"}
"list_clear"    "clears selected list"      "0004%00e%1"  2 2   {B  "type (100=cal, 104=todo, 108=cont)"}   {B "list (155=todo, 150=cont)"}
"list_create"   "creates list item"         "0004%00d%10b%2%3%4"  2   5 {B  "type (100=cal, 104=todo, 108=cont)"}   {B "list (155=todo, 150=cont)"} {B "length +1"} {B "length"} {S "name" 1 20}

"commit"    "commits transaction"   "0004%02001"    2   1   {B  "type (100=cal, 104=todo, 108=cont)"}

"todo_test1" "testing command"              "000468%09b0a0103" 2 1 {B "command"}
"todo_test2" "testing command"              "0004680c9b%00103" 2 1 {B "sub command"}
"todo_test" "testing command"               "000468%09b%10103" 1 2 {B "command"} {B "sub command"}
