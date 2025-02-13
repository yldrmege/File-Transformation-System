#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/xmlschemastypes.h>
#include <libxml/tree.h>

typedef struct  {
    char dataFileName[50];
    int keyStart;
    int keyEnd;
    char order[5];
}JsonInfo;

typedef struct{
   char littleend[9];
   char bigend[9];
   int decimal;
  
}CourseSurveyRating;

typedef struct {
    char name[20];
    char surname[30];
    char stuID[11];
    char gender;
    char email[100];
    char phone[20];
    char letter_grade[3];
    int midterm;
    int project;
    int final;
    char regularStudent[5];
    int course_surveyRating;
} Student;

int writeBinaryFromCSV(const char *csvFilename, const char *binaryFilename,char *title[]);
void readBinaryToStudents(const char *binaryFilename, Student *students, int *count);
char **readKeyRangeValues(const char *binaryFilename, int keyStart, int keyEnd, int *numValues) ;
json_object* ReadJsonFile(const char *filename);
JsonInfo* JsonParsetoStruct(json_object *jobj);
void SortingStudents(char **values, Student *students, int numValues,char* Order);
char* getHex(int n, char *format);
int hexToDecimal(char *hex);
void getValueRatings(CourseSurveyRating *ratings,Student *students,int arraySize);
void createXmlFromArray(const char *xmlFilename, char *title[], Student *students,CourseSurveyRating *ratings, int studentCount);
void Validation(char* xmlFileName,char* xsdFileName);

int main(int argc,char**argv) {
     if (argc < 3 || argc > 4) {// it needs to just 3 arg or 4 arg
        return 1;
    }
    char *inputFile;
    char *outputFile;
    int type;

    
    if (argc == 3) {// just input file and type
        inputFile = argv[1];
        type= atoi(argv[2]);
    }
    
    else if (argc == 4) {// input file,output file and type
        inputFile = argv[1];
        outputFile= argv[2];
        type = atoi(argv[3]);
    }

    char* typeSituation1;
    char* typeSituation2;
    if(strcmp(inputFile,"records.xml")==0){// if inputfile=records.xml,WriteBinaryfromCSV function should be executed so situations are default values.
        typeSituation1="records.csv";
        typeSituation2="records.dat";
    }
    else{
        typeSituation1=inputFile;
        typeSituation2=outputFile;
    }

    int studentCount;

    char* title[12];//tags

    int array_Size=writeBinaryFromCSV(typeSituation1,typeSituation2,title);//Read to Csv and return the number of the rows

    Student studentsFromBinary[array_Size];      
    
    readBinaryToStudents(typeSituation2, studentsFromBinary, &studentCount);// to store in students array
    
        
    if(type!=1){// records.xml 2
    const char *file_name = "setupParams.json";    
    
    json_object *jobj = ReadJsonFile(file_name);
    if (jobj == NULL) {
        return 1;
    }
    JsonInfo *jsonData = JsonParsetoStruct(jobj);
    

    int numValues;
    // read Binary file according to the values in Json(for key start and key end) store in string array
    char **values = readKeyRangeValues(jsonData->dataFileName,jsonData->keyStart,jsonData->keyEnd, &numValues);

    SortingStudents(values,studentsFromBinary,numValues,jsonData->order);//sort according to json file's order attribute('ASC' or 'DESC)

    CourseSurveyRating val_rating[array_Size];//this struct is for store hexVal_Bigend,hexVal_LittleEnd and Decimal

    getValueRatings(val_rating,studentsFromBinary,array_Size);   

    /* The student array, the title array in string format, and the val_rating array in coursesurveyrating type 
    are the information required for xml and are sent to the function.*/
    createXmlFromArray(inputFile,title,studentsFromBinary,val_rating,array_Size);
    if(type!=1&&type!=2){// records.xml records.xsd 3
        Validation(inputFile,outputFile);
    }
    free(values);
    free(jsonData);  // allocated memories are freed with the free command
    json_object_put(jobj);}

    return 0;
}

int writeBinaryFromCSV(const char *csvFilename, const char *binaryFilename,char *title[]) {
    FILE *csvFile = fopen(csvFilename, "r");
    if (csvFile == NULL) {
        printf("Error CSV file.\n");
        return 1; 
    }

    char buffer[1024];
    fgets(buffer, sizeof(buffer), csvFile);// read the first line
    char *token;
    int tokenIndex = 0;
    char *tempBuffer = strdup(buffer);
    char *temp = tempBuffer;
    while ((token = strsep(&temp, ",")) != NULL && tokenIndex < 12) { // we have 12 tags in csv so we need to store in array   
    size_t len = strlen(token);
    if (len > 0 && token[len - 1] == '\n')
        token[len - 1] = '\0';

    title[tokenIndex] = strdup(token);
    tokenIndex++;
    }
    free(tempBuffer);

   
    
    Student student;
    int studentCount = 0;

    FILE *binaryFile = fopen(binaryFilename, "wb");
    if (binaryFile == NULL) {
        printf("Error for writing.\n");
        fclose(csvFile);
        return 1;
    }

    while (fgets(buffer, sizeof(buffer), csvFile) != NULL) {
        char *tempBuffer = strdup(buffer); 
        char *temp = tempBuffer;

        char *token;
        int tokenIndex = 0;
        /*Unlike strtok, the strsep function assigns a default value instead of skipping it if there is no value between two commas.*/
        while ((token = strsep(&temp, ",")) != NULL) {
            switch (tokenIndex) {
                case 0: strcpy(student.name, token); break;
                case 1: strcpy(student.surname, token); break;
                case 2: strcpy(student.stuID, token); break;
                case 3: student.gender = token[0]; break;
                case 4: strcpy(student.email, token); break;
                case 5: strcpy(student.phone, token); break;
                case 6: strcpy(student.letter_grade, token); break;
                case 7: student.midterm = atoi(token); break;
                case 8: student.project = atoi(token); break;
                case 9: student.final = atoi(token); break;
                case 10: strcpy(student.regularStudent, token); break;
                case 11: student.course_surveyRating = atoi(token); break;
            }
            tokenIndex++;
        }

        fwrite(&student, sizeof(Student), 1, binaryFile);
        studentCount++;

        free(tempBuffer); 
    }

    fclose(csvFile);
    fclose(binaryFile);
    return studentCount;// return for array_Size
}

void readBinaryToStudents(const char *binaryFilename, Student *students, int *count) {
    FILE *binaryFile = fopen(binaryFilename, "rb");
    if (binaryFile == NULL) {
        printf("Error to read binary\n");
        return;
    }

    fseek(binaryFile, 0, SEEK_END);
    long fileSize = ftell(binaryFile);
    *count = fileSize / sizeof(Student);
    rewind(binaryFile);

    fread(students, sizeof(Student), *count, binaryFile);
    fclose(binaryFile);
}
char **readKeyRangeValues(const char *binaryFilename, int keyStart, int keyEnd, int *numValues) {
    FILE *binaryFile = fopen(binaryFilename, "rb");
    if (binaryFile == NULL) {
        printf("Error to read binary\n");
        return NULL;
    }

    fseek(binaryFile, 0, SEEK_END);
    long fileSize = ftell(binaryFile);
    rewind(binaryFile);

   
    Student student;

    
    char **keyRangeValues = NULL;
    *numValues = 0;

    /*According to the variables specified as keyStart and keyEnd in the Json File, the range in the binary file is read and assigned to the array.*/
    while (fread(&student, sizeof(Student), 1, binaryFile) == 1) {
        char *keyRangeValue = (char *)malloc(keyEnd - keyStart + 2); // +2: for '\0' and end - start + 1

        
        char *startPtr = student.name + keyStart - 1;
        char *endPtr = student.name + keyEnd;
        int length = endPtr - startPtr;
        strncpy(keyRangeValue, startPtr, length);
        keyRangeValue[length] = '\0'; // add NULL Terminated
        /*The realloc function is used to change the size of the memory block. 
        This function is used to increase or decrease the size of a previously allocated memory block.*/
        keyRangeValues = (char **)realloc(keyRangeValues, (*numValues + 1) * sizeof(char *));
        keyRangeValues[*numValues] = keyRangeValue;
        (*numValues)++;
    }
     fclose(binaryFile);

    return keyRangeValues;
}


json_object* ReadJsonFile(const char *filename) {//Read the Json File
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error to open Json File\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *filecontent = (char *)malloc(filesize + 1);
    fread(filecontent, 1, filesize, file);
    fclose(file);
    filecontent[filesize] = '\0';

    json_object *jobj = json_tokener_parse(filecontent);
    free(filecontent);

    if (jobj == NULL) {
        fprintf(stderr, "Error to Parse Json because is null\n");
        return NULL;
    }

    return jobj;
}

 JsonInfo* JsonParsetoStruct(json_object *jobj) {
    enum json_type type;
    JsonInfo *data = (JsonInfo*)malloc(sizeof( JsonInfo));
    /*Same logic as classic json parse. The only difference is that if there are other variables in the json files, it will not be able to import them.*/
    json_object_object_foreach(jobj, key, val) {
        type = json_object_get_type(val);
        if (strcmp(key, "dataFileName") == 0 && type == json_type_string)
            strcpy(data->dataFileName, json_object_get_string(val));
        else if (strcmp(key, "keyStart") == 0 && type == json_type_int)
            data->keyStart = json_object_get_int(val);
        else if (strcmp(key, "keyEnd") == 0 && type == json_type_int)
            data->keyEnd = json_object_get_int(val);
        else if (strcmp(key, "order") == 0 && type == json_type_string)
            strcpy(data->order, json_object_get_string(val));
    }

    return data;
}
void SortingStudents(char **values, Student *students, int numValues,char* Order) {//Using Classic InsertionSort method (With information from CME2204 course)
    if(strcmp(Order,"ASC")==0){
        for (int i = 1; i < numValues; i++) {
        char *keyvalue = values[i];
        Student keystudent = students[i];
        int j = i - 1;
        
       
        while (j >= 0 && strcmp(values[j], keyvalue) > 0) {
            values[j + 1] = values[j];
            students[j + 1] = students[j];
            j = j - 1;
        }
        values[j + 1] = keyvalue;
        students[j + 1] = keystudent;
      } 
    }
    else{
        for (int i = numValues - 2; i >= 0; i--) {
        char *keyvalue = values[i];
        Student keystudent = students[i];
        int j = i + 1;
        
        
        while (j < numValues && strcmp(keyvalue, values[j]) < 0) {
            values[j - 1] = values[j];
            students[j - 1] = students[j];
            j = j + 1;
        }
        values[j - 1] = keyvalue;
        students[j - 1] = keystudent;
      }
    }
    
}
char* getHex(int n, char *format) {//The number in decimal format is changed to 8 nibbles.
    static char hex[9];
    hex[8] = '\0';
    if (strcmp(format, "L") == 0) {
        for (int i = 0; i < 4; i++) {
            sprintf(hex + i * 2, "%02X", (n >> (i * 8)) & 0xFF);
        }
    } else if (strcmp(format, "B") == 0) {
        for (int i = 3; i >= 0; i--) {
            sprintf(hex + (3 - i) * 2, "%02X", (n >> (i * 8)) & 0xFF);
        }
    }
    return hex;
}

int hexToDecimal(char *hex) {
    int decimalNumber = 0;
    int length = strlen(hex);

    for (int i = 0; i < length; i++) {   
        decimalNumber = decimalNumber * 16 + hex[i]-'0';      
    }
    return decimalNumber;
}
void getValueRatings(CourseSurveyRating *ratings,Student *students,int arraySize){   
    for(int i=0;i<arraySize;i++){
        strcpy(ratings[i].littleend, getHex(students[i].course_surveyRating, "L"));
        strcpy(ratings[i].bigend, getHex(students[i].course_surveyRating, "B"));
        ratings[i].decimal=hexToDecimal(ratings[i].littleend);
    }
}
void createXmlFromArray(const char *xmlFilename, char *title[], Student *students, CourseSurveyRating *ratings, int studentCount) {
    /*Thanks to the sprintf function, non-string values ​​are written to XML as char* with the help of an array.*/
    xmlDocPtr doc = NULL;       /* document pointer */
    xmlNodePtr root_node = NULL;  /* node pointers */
    char buff[10];//for integer values

    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "records");
    xmlDocSetRootElement(doc, root_node);

    for (int i = 0; i < studentCount; i++) {
        xmlNodePtr row_node = xmlNewChild(root_node, NULL, BAD_CAST "row", NULL);
        sprintf(buff, "%d", i + 1);
        xmlNewProp(row_node, BAD_CAST "id", BAD_CAST buff);

        xmlNodePtr student_info_node = xmlNewChild(row_node, NULL, BAD_CAST "student_info", NULL);
        xmlNewChild(student_info_node, NULL, BAD_CAST title[0], BAD_CAST students[i].name);
        xmlNewChild(student_info_node, NULL, BAD_CAST title[1], BAD_CAST students[i].surname);
        xmlNewChild(student_info_node, NULL, BAD_CAST title[2], BAD_CAST students[i].stuID);
        char genderstr[2];// for gender


        sprintf(genderstr, "%c", students[i].gender);

        xmlNewChild(student_info_node, NULL, BAD_CAST title[3], BAD_CAST genderstr);
        xmlNewChild(student_info_node, NULL, BAD_CAST title[4], BAD_CAST students[i].email);
        xmlNewChild(student_info_node, NULL, BAD_CAST title[5], BAD_CAST students[i].phone);

        xmlNodePtr grade_info_node = xmlNewChild(row_node, NULL, BAD_CAST "grade_info", NULL);
        xmlNewProp(grade_info_node, BAD_CAST title[6], BAD_CAST students[i].letter_grade);
        sprintf(buff, "%d", students[i].midterm); 
        xmlNewChild(grade_info_node, NULL, BAD_CAST title[7], BAD_CAST buff);
        sprintf(buff, "%d", students[i].project); 
        xmlNewChild(grade_info_node, NULL, BAD_CAST title[8], BAD_CAST buff);
        sprintf(buff, "%d", students[i].final); 
        xmlNewChild(grade_info_node, NULL, BAD_CAST title[9], BAD_CAST buff);

        xmlNewChild(row_node, NULL, BAD_CAST title[10], BAD_CAST students[i].regularStudent);

        xmlNodePtr course_surveyRating_node = xmlNewChild(row_node, NULL, BAD_CAST title[11], NULL);
        sprintf(buff, "%d", students[i].course_surveyRating);

        xmlNodeSetContent(course_surveyRating_node, BAD_CAST buff);
        xmlNewProp(course_surveyRating_node, BAD_CAST "hexVal_bigEnd", BAD_CAST ratings[i].littleend);
        xmlNewProp(course_surveyRating_node, BAD_CAST "hexVal_littleEnd", BAD_CAST ratings[i].bigend);
        sprintf(buff, "%d", ratings[i].decimal); 
        xmlNewProp(course_surveyRating_node, BAD_CAST "decimal", BAD_CAST buff);
    }

    xmlSaveFormatFileEnc(xmlFilename, doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    
    xmlCleanupParser();
}
void Validation(char* xmlFileName, char* xsdFileName){
    xmlDocPtr doc;
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt;

    xmlLineNumbersDefault(1); //set line numbers, 0> no substitution, 1>substitution
    ctxt = xmlSchemaNewParserCtxt(xsdFileName); //create an xml schemas parse context
    schema = xmlSchemaParse(ctxt); //parse a schema definition resource and build an internal XML schema
    xmlSchemaFreeParserCtxt(ctxt); //free the resources associated to the schema parser context

    doc = xmlReadFile(xmlFileName, NULL, 0); //parse an XML file
    if (doc == NULL)
    {
        fprintf(stderr, "Could not parse %s\n", xmlFileName);
    }
    else
    {
        xmlSchemaValidCtxtPtr ctxt;  //structure xmlSchemaValidCtxt, not public by API
        int ret;

        ctxt = xmlSchemaNewValidCtxt(schema); //create an xml schemas validation context 
        ret = xmlSchemaValidateDoc(ctxt, doc); //validate a document tree in memory
        if (ret == 0) //validated
        {
            printf("%s validates\n", xmlFileName);
        }
        else if (ret > 0) //positive error code number
        {
            printf("%s fails to validate\n", xmlFileName);
        }
        else //internal or API error
        {
            printf("%s validation generated an internal error\n",xmlFileName);
        }
        xmlSchemaFreeValidCtxt(ctxt); //free the resources associated to the schema validation context
        xmlFreeDoc(doc);
    }
    // free the resource
    if(schema != NULL)
        xmlSchemaFree(schema); //deallocate a schema structure

    xmlSchemaCleanupTypes(); //cleanup the default xml schemas types library
    xmlCleanupParser(); //cleans memory allocated by the library itself 
    xmlMemoryDump(); //memory dump
}

