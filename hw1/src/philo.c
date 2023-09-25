#include <stdlib.h>

#include "global.h"
#include "debug.h"


double stringToDouble(const char* str){
    double result = 0.0;
    double fraction = 0.1;

    while(*str >= '0' && *str <= '9') {
        result = result * 10.0 + (*str - '0');
        str++;
    }

    if(*str == '.'){
        str++;
        while(*str >= '0' && *str <= '9'){
            result += (*str - '0') * fraction;
            fraction /= 10.0;
            str++;
        }
    }

    return result;
}


int isSymmetric(int rowCounter){
    for(int i = 0; i < rowCounter; i++){
        if(*(*(distances + i) + i) != 0){
            return 0;
        }
    }

    return 1;
}

void printDistances(){
    for(int i = 0; i < num_all_nodes; i++){
        for(int j = 0; j < num_all_nodes; j++){
            printf("%lf,",*(*(distances+j)+i));
        }
        printf("\n");
    }
}

void printNames(){
    for(int i = 0; i < num_all_nodes; i++){
        printf("%s\n", *(node_names+i));
    }
}

void printActiveNodeNames(){
    for(int i =0; i < num_active_nodes; i++){
        printf("%s\n", *(node_names + *(active_node_map + i)));
    }
}

void printActiveNodeMap(){
    for(int i = 0; i < num_active_nodes; i++){
        printf("%d\n", *(active_node_map + i));
    }
}

void updateSums(){
    double currentSum = 0;
    for(int i =0; i < num_all_nodes; i++){
        for(int j =0; j < num_all_nodes; j++){
            currentSum = currentSum + *(*(distances + i)+j);
        }
        *(row_sums+i) = currentSum;
    }
}

double getDistance(int x, int y){
    return *(*(distances + x)+ y);
}

double getActiveSum(int index){
    double tempSum = 0;
    for(int i = 0; i < num_active_nodes; i++){
        tempSum = tempSum + getDistance(index, *(active_node_map+i));
    }
    return tempSum;
}

void intToString(int num){
    int i = 1;
    *(*(node_names + num)) = '#';
    do {
        *(*(node_names + num) + i) = num % 10 + '0';
        num /= 10;
        i++;
    } while (num > 0);

    *(*(node_names + num) + i) = '\0';
    // int len = i;
    // for(int j = 0; j < len /2; j++){
    //     char temp = *(*(node_names + num)+j);
    //     *(*(node_names + num) + j) = *(*(node_names + num) + len - 1 - j);
    //     *(*(node_names + num) + len - 1 - j) = temp;
    // }
}

void updateDistancesNewNode(int newIndexNode, int xIndexOfMin, int yIndexOfMin){
    printf("%s%d\n", "newIndexNode: ", newIndexNode);
    *(*(distances + newIndexNode) + newIndexNode) = 0;
    for(int i = 0; i < num_all_nodes; i++){
        *(*(distances + newIndexNode) + i) = 0.5 * (getDistance(xIndexOfMin,i) + getDistance(yIndexOfMin,i) - getDistance(xIndexOfMin,yIndexOfMin));
        *(*(distances + i) + newIndexNode) = *(*(distances + newIndexNode) + i);
    }
}

int findIndexOfMap(int rawIndex){
    for(int i = 0; i < num_active_nodes; i++){
        if(*(active_node_map + i) == rawIndex){
            return i;
        }
    }
    return -1;
}

void printNodeNeighbors(){
    for(int i = 0; i < num_all_nodes; i++){
            printf("%s%s\n", "Current Node: ", *(node_names + i));
            NODE currentNode = *(nodes + i);
            for(int j = 0; j < 3; j++){
                // printf("%s\n", "1");
                NODE *currentNeighbor = *(currentNode.neighbors + j);
                // printf("%s\n", "2");
                if(currentNeighbor != NULL){
                    printf("%s[%d]: %s\n", "Neighbor Nodes", j, currentNeighbor->name);
                }
        }
    }
}

void globalStatus(){
    printf("%s\n", "----------Status check----------");
    printActiveNodeNames();
    printActiveNodeMap();
    printDistances();
    printNames();
    printf("%s%d\n", "num_taxa: ", num_taxa);
    printf("%s%d\n", "num_active_nodes: ", num_active_nodes);
    printf("%s%d\n", "num_all_nodes: ", num_all_nodes);
    printf("%s\n", "------------END----------------");

}

/**
 * @brief  Read genetic distance data and initialize data structures.
 * @details  This function reads genetic distance data from a specified
 * input stream, parses and validates it, and initializes internal data
 * structures.
 *
 * The input format is a simplified version of Comma Separated Values
 * (CSV).  Each line consists of text characters, terminated by a newline.
 * Lines that start with '#' are considered comments and are ignored.
 * Each non-comment line consists of a nonempty sequence of data fields;
 * each field iis terminated either by ',' or else newline for the last
 * field on a line.  The constant INPUT_MAX specifies the maximum number
 * of data characters that may be in an input field; fields with more than
 * that many characters are regarded as invalid input and cause an error
 * return.  The first field of the first data line is empty;
 * the subsequent fields on that line specify names of "taxa", which comprise
 * the leaf nodes of a phylogenetic tree.  The total number N of taxa is
 * equal to the number of fields on the first data line, minus one (for the
 * blank first field).  Following the first data line are N additional lines.
 * Each of these lines has N+1 fields.  The first field is a taxon name,
 * which must match the name in the corresponding column of the first line.
 * The subsequent fields are numeric fields that specify N "distances"
 * between this taxon and the others.  Any additional lines of input following
 * the last data line are ignored.  The distance data must form a symmetric
 * matrix (i.e. D[i][j] == D[j][i]) with zeroes on the main diagonal
 * (i.e. D[i][i] == 0).
 *
 * If 0 is returned, indicating data successfully read, then upon return
 * the following global variables and data structures have been set:
 *   num_taxa - set to the number N of taxa, determined from the first data line
 *   num_all_nodes - initialized to be equal to num_taxa
 *   num_active_nodes - initialized to be equal to num_taxa
 *   node_names - the first N entries contain the N taxa names, as C strings
 *   distances - initialized to an NxN matrix of distance values, where each
 *     row of the matrix contains the distance data from one of the data lines
 *   nodes - the "name" fields of the first N entries have been initialized
 *     with pointers to the corresponding taxa names stored in the node_names
 *     array.
 *   active_node_map - initialized to the identity mapping on [0..N);
 *     that is, active_node_map[i] == i for 0 <= i < N.
 *
 * @param in  The input stream from which to read the data.
 * @return 0 in case the data was successfully read, otherwise -1
 * if there was any error.  Premature termination of the input data,
 * failure of each line to have the same number of fields, and distance
 * fields that are not in numeric format should cause a one-line error
 * message to be printed to stderr and -1 to be returned.
 */

int read_distance_data(FILE *in) {
    // fprintf(stderr, "%s\n", "Entered read distances data function");
    // TO BE IMPLEMENTED

    int ch;
    int rowCounter = 0;
    int columnCounter = 0;
    int bufferCounter = 0;

    while((ch = fgetc(in)) != '\0'){                            // Reading a file by single character
        // printf("%s%c\n", "current character:",ch);
                                            // Checks if the line starts with #
        while(ch == '#'){                                       // Skip the line
            while(ch != '\n'){
                ch = fgetc(in);
                // printf("%s%c\n", "current character:",ch);
            }
            ch = fgetc(in);
            // printf("%s%c\n", "current character:",ch);
        }
        // printf("%s\n", "No more # line");

        while(ch != '\n' && rowCounter == 0 && ch != '\0'){                      // While loop only for the first line
            while(ch != ',' && ch != '\n' && columnCounter != 0){                                       // While loop for each columns
                *(input_buffer + bufferCounter) = ch;

                if(bufferCounter == INPUT_MAX){                    // If the length of field is longer than maximum input length then return error
                    // printf("%s\n", "taxa input overflow");
                    return -1;
                }
                bufferCounter++;
                ch = fgetc(in);
                // printf("%s%c\n", "current character:",ch);
            }

            if(bufferCounter != 0){                                 // Skip if the field(buffer) is empty
                num_taxa++;
                *(input_buffer + bufferCounter) = '\0';             // Set null character at the end of buffer
                for(int i = 0; i <= bufferCounter; i++){            // Assigning each characters in a buffer to node_names[column]
                    *(*(node_names + (columnCounter-1)) + i) = *(input_buffer + i);
                }
            }

            columnCounter++;
            bufferCounter = 0;

            if(ch != '\n'){
                ch = fgetc(in);
                // printf("%s%c\n", "current character:",ch);
                // printf("%s\n", "NEXT COLUMN");
            }
        }

        while(ch != '\n' && rowCounter != 0 && ch != '\0'){
            // printf("%s%d\n", "rowCounter: ",rowCounter);
            // rows after first row
            while(ch != ',' && ch != '\n'){                               // Looping by each column
                if((ch >= '0' && ch <= '9' && columnCounter != 0) || ch == '.' ){
                    *(input_buffer + bufferCounter) = ch;
                    if(bufferCounter == INPUT_MAX){
                        // printf("%s\n", "distance input overflow");
                        return -1;
                    }
                    bufferCounter++;
                    ch = fgetc(in);
                    // printf("%s%c\n", "current character:",ch);
                }else if(columnCounter == 0){
                    ch = fgetc(in);
                    // printf("%s%c\n", "current character:",ch);
                }else{
                    // printf("%s\n", "Invalid input");
                    return -1;
                }
            }

            if(bufferCounter != 0 && columnCounter != 0){
                *(input_buffer + bufferCounter) = '\0';
                double tempDistance = stringToDouble(input_buffer);             // Is it passing by value or ref?
                *(*(distances + (rowCounter - 1)) + (columnCounter - 1)) = tempDistance;
                // Is is fine even though I change the value of tempDistance later?
            }

            columnCounter++;
            bufferCounter = 0;
            if(ch != '\n' && ch != '\0'){
                ch = fgetc(in);
                // printf("%s%c\n", "current character:",ch);
                // printf("%s\n", "NEXT COLUMN");
            }
        }

        columnCounter = 0;
        if(rowCounter == num_taxa) break;
        rowCounter++;
        // printf("%s\n", "--NEXT ROW");
    }

    for(int i = 0; i < num_taxa; i++){              // Setting names of nodes from node_names and active_node_map
        (nodes + i) -> name = *(node_names + i);
        *(active_node_map + i) = i;
    }

    num_all_nodes = num_taxa;
    num_active_nodes = num_taxa;

    if(isSymmetric(rowCounter) == 0){
        // printf("%s\n", "not symmetric");
        return -1;
    }
    // printDistances();
    // printNames();

    return 0;
}

int isMatchingString(const char *str1, const char *str2){
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return 0; // Characters do not match
        }
        str1++;
        str2++;
    }

    // If both strings have reached the end simultaneously, they match
    return (*str1 == '\0' && *str2 == '\0');
}

int checkOutlier(){

    int indexTemp= -1;

    for (int i = 0; i < num_taxa; i++) {
        if (isMatchingString(outlier_name, *(node_names + i))) {
            printf("Found a matching string: %s\n", *(node_names + i));
            indexTemp = i;
            break; // If you want to stop after the first match
        }
    }

    return indexTemp;
}




// int isLeaf(NODE root){
//     for(int i = 0; i < 3; i++){
//         if(*(root.neighbors + i)){

//         }
//     }
// }

/**
 * @brief  Emit a representation of the phylogenetic tree in Newick
 * format to a specified output stream.
 * @details  This function emits a representation in Newick format
 * of a synthesized phylogenetic tree to a specified output stream.
 * See (https://en.wikipedia.org/wiki/Newick_format) for a description
 * of Newick format.  The tree that is output will include for each
 * node the name of that node and the edge distance from that node
 * its parent.  Note that Newick format basically is only applicable
 * to rooted trees, whereas the trees constructed by the neighbor
 * joining method are unrooted.  In order to turn an unrooted tree
 * into a rooted one, a root will be identified according by the
 * following method: one of the original leaf nodes will be designated
 * as the "outlier" and the unique node adjacent to the outlier
 * will serve as the root of the tree.  Then for any other two nodes
 * adjacent in the tree, the node closer to the root will be regarded
 * as the "parent" and the node farther from the root as a "child".
 * The outlier node itself will not be included as part of the rooted
 * tree that is output.  The node to be used as the outlier will be
 * determined as follows:  If the global variable "outlier_name" is
 * non-NULL, then the leaf node having that name will be used as
 * the outlier.  If the value of "outlier_name" is NULL, then the
 * leaf node having the greatest total distance to the other leaves
 * will be used as the outlier.
 *
 * @param out  Stream to which to output a rooted tree represented in
 * Newick format.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.  If the global variable "outlier_name" is
 * non-NULL, then it is an error if no leaf node with that name exists
 * in the tree.
 */

// void printNodeNeighbors(){
//     for(int i = 0; i < num_all_nodes; i++){
//             printf("%s%s\n", "Current Node: ", *(node_names + i));
//             NODE currentNode = *(nodes + i);
//             for(int j = 0; j < 3; j++){
//                 // printf("%s\n", "1");
//                 NODE *currentNeighbor = *(currentNode.neighbors + j);
//                 // printf("%s\n", "2");
//                 if(currentNeighbor != NULL){
//                     printf("%s[%d]: %s\n", "Neighbor Nodes", j, currentNeighbor->name);
//                 }
//         }
//     }
// }

// int getIndexByNameHelper(NODE temp, int i){
//     char *tempStr = *(node_names + i);
//     while(*(temp.name) != '\0' && *(tempStr) != '\0'){
//         if(*(temp.name) != *(*(node_names + i))){
//             break;
//         }
//         temp.name++;
//         tempStr++;
//     }
//     return (*(temp.name) != '\0' && *(tempStr) != '\0');
// }

int getIndexByName(NODE temp){
    int returnIndex = -1;
    for(int i = 0; i < num_all_nodes; i++){
        char *tempStr = *(node_names + i);
        if(isMatchingString(temp.name, tempStr)){
            returnIndex = i;
        }
    }
    return returnIndex;
}

double getDistanceByNames(NODE root, NODE *currentNeighbor){
    int indexOfRoot = getIndexByName(root);
    int indexOfNeighbor = getIndexByName(*currentNeighbor);
    return getDistance(indexOfRoot, indexOfNeighbor);

}



void makeNewickTree(FILE *out, NODE parent, NODE child){
    int a = -1;
    int b = -1;
    // fprintf(out, "%s\n" ,"beginofloop");
    for(int i = 0; i < 3; i++){
        // printf("%s\n", "1");
        NODE *temp = *(child.neighbors + i);

        if(temp != NULL){
            if(!isMatchingString(parent.name, temp->name)){
                // printf("%s\n", "2");
                if(a == -1){
                    // printf("%s\n", "setting a");
                    a = i;
                }else if(b == -1){
                    // printf("%s\n", "setting b");
                    b = i;
                }
            }
        }
        // printf("%s\n", "3");
    }
    // fprintf(out, "%s\n" ,"endofloop");

    if(a == -1 && b == -1) return;

    // fprintf(out, "%s\n" ,"1");

    NODE *nodeA = *(child.neighbors + a);
    NODE *nodeB = *(child.neighbors + b);

    // fprintf(out, "%s\n" ,"2");


    fprintf(out, "%s", "(");
    makeNewickTree(out, child, *(*(child.neighbors + a)));
    fprintf(out, "%s:%.2lf", nodeA->name, getDistanceByNames(child, nodeA));
    fprintf(out, ",");
    makeNewickTree(out, child, *(*(child.neighbors + b)));
    fprintf(out, "%s:%.2lf", nodeB->name, getDistanceByNames(child, nodeB));
    fprintf(out, ")");


}

void setNewickTree(FILE *out, NODE root){

    NODE *beginningNode = *(root.neighbors);
    makeNewickTree(out, root, *(beginningNode));
    fprintf(out, "%s:%.2lf\n", beginningNode->name, getDistanceByNames(root, beginningNode));
}



int findLongestNodeIndex(){
    double temp = *(*(distances));
    int x;
    for(int i = 0; i < num_taxa; i++){
        for(int j = 0; j < num_taxa; j++){
            if(temp < *(*(distances + i) + j)){
                temp = *(*(distances + i) + j);
                x = i;
            }
        }
    }

    return x;
}

int emit_newick_format(FILE *out) {
    int indexOfOutlier = -1;
    if(num_taxa == 1) return 0;
    if(num_taxa == 2){
        double singleD = getDistance(0, 1);
        fprintf(out, "%s:%.2lf", *(node_names),singleD);
    }
    if(outlier_name != NULL){
        if(checkOutlier() == -1){
            return -1;
        }
        indexOfOutlier = checkOutlier();
    }

    while(num_active_nodes > 2){
        double minimumQ = 0;
        double temp = 0;
        int xIndexOfMin = 0;
        int yIndexOfMin = 0;
        int a;
        int b;
        updateSums();
        // printf("%s\n", "1");

        for(int i = 0; i < num_active_nodes; i++){
            for(int j = 0; j < num_active_nodes; j++){
                if(j != i){
                    a = *(active_node_map+i);
                    b = *(active_node_map+j);
                    temp = (num_active_nodes - 2) * getDistance(a, b) - getActiveSum(a) - getActiveSum(b);
                    // printf("%s\n", "2");

                    if(i == 0 && j == 0){
                        minimumQ = temp;
                        xIndexOfMin = a;
                        yIndexOfMin = b;
                    }
                    if(minimumQ > temp){
                        minimumQ = temp;
                        xIndexOfMin = a;
                        yIndexOfMin = b;
                    }
                }
            }
        }
        // printf("%s\n", "END of for loop");
        // printf("%s%d\n", "xIndexOfMin: ", xIndexOfMin);
        // printf("%s%d\n", "yIndexOfMin: ", yIndexOfMin);
        // printf("%s%lf\n", "minimumQ: ", minimumQ);

        // double xToNewDistance = 0.5 * getDistance(xIndexOfMin, yIndexOfMin) + 1/(2*(num_active_nodes - 2)) * (getActiveSum(xIndexOfMin) - getActiveSum(yIndexOfMin));
        double xToNewDistance = (getDistance(xIndexOfMin,yIndexOfMin) + (getActiveSum(xIndexOfMin) - getActiveSum(yIndexOfMin)) / (num_active_nodes - 2)) / 2;
        double yToNewDistance = getDistance(xIndexOfMin, yIndexOfMin) - xToNewDistance;
        int newIndexNode = num_all_nodes;
        int copyOfNewIndex = newIndexNode;
        intToString(copyOfNewIndex);
        // printf("%s\n", "3");
        (nodes + newIndexNode) -> name = *(node_names + newIndexNode);

        *((nodes + newIndexNode)->neighbors) = (nodes + xIndexOfMin);
        *((nodes + newIndexNode)->neighbors + 1) = (nodes + yIndexOfMin);

        for(int i = 0; i < 3; i++){
            if(*((nodes + xIndexOfMin)->neighbors + i) == NULL){
                *((nodes + xIndexOfMin)->neighbors + i) = (nodes + newIndexNode);
                break;
            }
        }

        for(int i = 0; i < 3; i++){
            if(*((nodes + yIndexOfMin)->neighbors + i) == NULL){
                *((nodes + yIndexOfMin)->neighbors + i) = (nodes + newIndexNode);
                break;
            }
        }

        // printf("%s%d\n", "newIndexNode: ", newIndexNode);
        *(*(distances + newIndexNode) + newIndexNode) = 0;
        for(int i = 0; i < num_active_nodes; i++){
            if(*(active_node_map + i) == xIndexOfMin){
                *(*(distances + newIndexNode) + *(active_node_map + i)) = xToNewDistance;
                *(*(distances + *(active_node_map + i)) + newIndexNode) = xToNewDistance;
            }else if(*(active_node_map + i) == yIndexOfMin){
                *(*(distances + newIndexNode) + *(active_node_map + i)) = yToNewDistance;
                *(*(distances + *(active_node_map + i)) + newIndexNode) = yToNewDistance;
            }else{
                *(*(distances + newIndexNode) + *(active_node_map + i)) = 0.5 * (getDistance(xIndexOfMin,*(active_node_map + i)) + getDistance(yIndexOfMin,*(active_node_map + i)) - getDistance(xIndexOfMin,yIndexOfMin));
                *(*(distances + *(active_node_map + i)) + newIndexNode) = *(*(distances + newIndexNode) + *(active_node_map + i));
            }
        }

        // fprintf(out, "%d,%d,%.2lf\n", xIndexOfMin,newIndexNode,xToNewDistance);
        // fprintf(out, "%d,%d,%.2lf\n", yIndexOfMin,newIndexNode,yToNewDistance);
        // printf("%d,%d,%.2lf\n", xIndexOfMin,newIndexNode,xToNewDistance);
        // printf("%d,%d,%.2lf\n", yIndexOfMin,newIndexNode,yToNewDistance);

        *(active_node_map + findIndexOfMap(xIndexOfMin)) = num_all_nodes;
        *(active_node_map + findIndexOfMap(yIndexOfMin)) = *(active_node_map + (num_active_nodes-1));
        num_all_nodes++;
        num_active_nodes--;

        // globalStatus();
    }

    if(num_active_nodes == 2){
        int aIndex = *(active_node_map);
        int bIndex = *(active_node_map + 1);

        for(int i = 0; i < 3; i++){
            if(*((nodes + aIndex)->neighbors + i) == NULL){
                *((nodes + aIndex)->neighbors + i) = (nodes + bIndex);
                break;
            }
        }
        for(int i = 0; i < 3; i++){
            if(*((nodes + bIndex)->neighbors + i) == NULL){
                *((nodes + bIndex)->neighbors + i) = (nodes + aIndex);
                break;
            }
        }

        // fprintf(out, "%d,%d,%.2lf\n", aIndex, bIndex, getDistance(aIndex, bIndex));
        // printf("%d,%d,%.2lf\n", aIndex, bIndex, getDistance(aIndex, bIndex));
    }

    if(outlier_name == NULL){
        indexOfOutlier = findLongestNodeIndex();
    }

    setNewickTree(out, *(nodes + indexOfOutlier));

    return 0;

}

/**
 * @brief  Emit the synthesized distance matrix as CSV.
 * @details  This function emits to a specified output stream a representation
 * of the synthesized distance matrix resulting from the neighbor joining
 * algorithm.  The output is in the same CSV form as the program input.
 * The number of rows and columns of the matrix is equal to the value
 * of num_all_nodes at the end of execution of the algorithm.
 * The submatrix that consists of the first num_leaves rows and columns
 * is identical to the matrix given as input.  The remaining rows and columns
 * contain estimated distances to internal nodes that were synthesized during
 * the execution of the algorithm.
 *
 * @param out  Stream to which to output a CSV representation of the
 * synthesized distance matrix.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.
 */

void printMatrix(FILE *out){
    fprintf(out, ",");
    for(int i = 0; i < num_all_nodes + 1; i++){
        for(int j = 0; j < num_all_nodes; j++){
            if(i == 0){
                if(j != num_all_nodes-1) fprintf(out, "%s,", *(node_names + j));
                else fprintf(out, "%s\n", *(node_names + j));
            }else{
                if(j != num_all_nodes-1) fprintf(out, "%.2lf,", *(*(distances + (i - 1)) + j));
                else fprintf(out, "%.2lf\n", *(*(distances + (i - 1)) + j));
            }
        }
        if(i != num_all_nodes){
            fprintf(out, "%s,", *(node_names + i));
        }
    }
}

int emit_distance_matrix(FILE *out) {

    while(num_active_nodes > 2){
        double minimumQ = 0;
        double temp = 0;
        int xIndexOfMin = 0;
        int yIndexOfMin = 0;
        int a;
        int b;
        updateSums();
        // printf("%s\n", "1");

        for(int i = 0; i < num_active_nodes; i++){
            for(int j = 0; j < num_active_nodes; j++){
                if(j != i){
                    a = *(active_node_map+i);
                    b = *(active_node_map+j);
                    temp = (num_active_nodes - 2) * getDistance(a, b) - getActiveSum(a) - getActiveSum(b);
                    // printf("%s\n", "2");

                    if(i == 0 && j == 0){
                        minimumQ = temp;
                        xIndexOfMin = a;
                        yIndexOfMin = b;
                    }
                    if(minimumQ > temp){
                        minimumQ = temp;
                        xIndexOfMin = a;
                        yIndexOfMin = b;
                    }
                }
            }
        }
        // printf("%s\n", "END of for loop");
        // printf("%s%d\n", "xIndexOfMin: ", xIndexOfMin);
        // printf("%s%d\n", "yIndexOfMin: ", yIndexOfMin);
        // printf("%s%lf\n", "minimumQ: ", minimumQ);

        // double xToNewDistance = 0.5 * getDistance(xIndexOfMin, yIndexOfMin) + 1/(2*(num_active_nodes - 2)) * (getActiveSum(xIndexOfMin) - getActiveSum(yIndexOfMin));
        double xToNewDistance = (getDistance(xIndexOfMin,yIndexOfMin) + (getActiveSum(xIndexOfMin) - getActiveSum(yIndexOfMin)) / (num_active_nodes - 2)) / 2;
        double yToNewDistance = getDistance(xIndexOfMin, yIndexOfMin) - xToNewDistance;
        int newIndexNode = num_all_nodes;
        int copyOfNewIndex = newIndexNode;
        intToString(copyOfNewIndex);
        // printf("%s\n", "3");
        (nodes + newIndexNode) -> name = *(node_names + newIndexNode);

        *((nodes + newIndexNode)->neighbors) = (nodes + xIndexOfMin);
        *((nodes + newIndexNode)->neighbors + 1) = (nodes + yIndexOfMin);

        for(int i = 0; i < 3; i++){
            if(*((nodes + xIndexOfMin)->neighbors + i) == NULL){
                *((nodes + xIndexOfMin)->neighbors + i) = (nodes + newIndexNode);
                break;
            }
        }

        for(int i = 0; i < 3; i++){
            if(*((nodes + yIndexOfMin)->neighbors + i) == NULL){
                *((nodes + yIndexOfMin)->neighbors + i) = (nodes + newIndexNode);
                break;
            }
        }

        // printf("%s%d\n", "newIndexNode: ", newIndexNode);
        *(*(distances + newIndexNode) + newIndexNode) = 0;
        for(int i = 0; i < num_active_nodes; i++){
            if(*(active_node_map + i) == xIndexOfMin){
                *(*(distances + newIndexNode) + *(active_node_map + i)) = xToNewDistance;
                *(*(distances + *(active_node_map + i)) + newIndexNode) = xToNewDistance;
            }else if(*(active_node_map + i) == yIndexOfMin){
                *(*(distances + newIndexNode) + *(active_node_map + i)) = yToNewDistance;
                *(*(distances + *(active_node_map + i)) + newIndexNode) = yToNewDistance;
            }else{
                *(*(distances + newIndexNode) + *(active_node_map + i)) = 0.5 * (getDistance(xIndexOfMin,*(active_node_map + i)) + getDistance(yIndexOfMin,*(active_node_map + i)) - getDistance(xIndexOfMin,yIndexOfMin));
                *(*(distances + *(active_node_map + i)) + newIndexNode) = *(*(distances + newIndexNode) + *(active_node_map + i));
            }
        }

        // fprintf(out, "%d,%d,%.2lf\n", xIndexOfMin,newIndexNode,xToNewDistance);
        // fprintf(out, "%d,%d,%.2lf\n", yIndexOfMin,newIndexNode,yToNewDistance);
        // printf("%d,%d,%.2lf\n", xIndexOfMin,newIndexNode,xToNewDistance);
        // printf("%d,%d,%.2lf\n", yIndexOfMin,newIndexNode,yToNewDistance);

        *(active_node_map + findIndexOfMap(xIndexOfMin)) = num_all_nodes;
        *(active_node_map + findIndexOfMap(yIndexOfMin)) = *(active_node_map + (num_active_nodes-1));
        num_all_nodes++;
        num_active_nodes--;

        // globalStatus();
    }

    if(num_active_nodes == 2){
        int aIndex = *(active_node_map);
        int bIndex = *(active_node_map + 1);

        for(int i = 0; i < 3; i++){
            if(*((nodes + aIndex)->neighbors + i) == NULL){
                *((nodes + aIndex)->neighbors + i) = (nodes + bIndex);
                break;
            }
        }
        for(int i = 0; i < 3; i++){
            if(*((nodes + bIndex)->neighbors + i) == NULL){
                *((nodes + bIndex)->neighbors + i) = (nodes + aIndex);
                break;
            }
        }

        // fprintf(out, "%d,%d,%.2lf\n", aIndex, bIndex, getDistance(aIndex, bIndex));
        // printf("%d,%d,%.2lf\n", aIndex, bIndex, getDistance(aIndex, bIndex));
    }

    printMatrix(out);
    printNodeNeighbors();
    return 0;
}

/**
 * @brief  Build a phylogenetic tree using the distance data read by
 * a prior successful invocation of read_distance_data().
 * @details  This function assumes that global variables and data
 * structures have been initialized by a prior successful call to
 * read_distance_data(), in accordance with the specification for
 * that function.  The "neighbor joining" method is used to reconstruct
 * phylogenetic tree from the distance data.  The resulting tree is
 * an unrooted binary tree having the N taxa from the original input
 * as its leaf nodes, and if (N > 2) having in addition N-2 synthesized
 * internal nodes, each of which is adjacent to exactly three other
 * nodes (leaf or internal) in the tree.  As each internal node is
 * synthesized, information about the edges connecting it to other
 * nodes is output.  Each line of output describes one edge and
 * consists of three comma-separated fields.  The first two fields
 * give the names of the nodes that are connected by the edge.
 * The third field gives the distance that has been estimated for
 * this edge by the neighbor-joining method.  After N-2 internal
 * nodes have been synthesized and 2*(N-2) corresponding edges have
 * been output, one final edge is output that connects the two
 * internal nodes that still have only two neighbors at the end of
 * the algorithm.  In the degenerate case of N=1 leaf, the tree
 * consists of a single leaf node and no edges are output.  In the
 * case of N=2 leaves, then no internal nodes are synthesized and
 * just one edge is output that connects the two leaves.
 *
 * Besides emitting edge data (unless it has been suppressed),
 * as the tree is built a representation of it is constructed using
 * the NODE structures in the nodes array.  By the time this function
 * returns, the "neighbors" array for each node will have been
 * initialized with pointers to the NODE structure(s) for each of
 * its adjacent nodes.  Entries with indices less than N correspond
 * to leaf nodes and for these only the neighbors[0] entry will be
 * non-NULL.  Entries with indices greater than or equal to N
 * correspond to internal nodes and each of these will have non-NULL
 * pointers in all three entries of its neighbors array.
 * In addition, the "name" field each NODE structure will contain a
 * pointer to the name of that node (which is stored in the corresponding
 * entry of the node_names array).
 *
 * @param out  If non-NULL, an output stream to which to emit the edge data.
 * If NULL, then no edge data is output.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.
 */




int build_taxonomy(FILE *out) {
    // globalStatus();
    while(num_active_nodes > 2){
        double minimumQ = 0;
        double temp = 0;
        int xIndexOfMin = 0;
        int yIndexOfMin = 0;
        int a;
        int b;
        updateSums();
        // printf("%s\n", "1");

        for(int i = 0; i < num_active_nodes; i++){
            for(int j = 0; j < num_active_nodes; j++){
                if(j != i){
                    a = *(active_node_map+i);
                    b = *(active_node_map+j);
                    temp = (num_active_nodes - 2) * getDistance(a, b) - getActiveSum(a) - getActiveSum(b);
                    // printf("%s\n", "2");

                    if(i == 0 && j == 0){
                        minimumQ = temp;
                        xIndexOfMin = a;
                        yIndexOfMin = b;
                    }
                    if(minimumQ > temp){
                        minimumQ = temp;
                        xIndexOfMin = a;
                        yIndexOfMin = b;
                    }
                }
            }
        }
        // printf("%s\n", "END of for loop");
        // printf("%s%d\n", "xIndexOfMin: ", xIndexOfMin);
        // printf("%s%d\n", "yIndexOfMin: ", yIndexOfMin);
        // printf("%s%lf\n", "minimumQ: ", minimumQ);

        // double xToNewDistance = 0.5 * getDistance(xIndexOfMin, yIndexOfMin) + 1/(2*(num_active_nodes - 2)) * (getActiveSum(xIndexOfMin) - getActiveSum(yIndexOfMin));
        double xToNewDistance = (getDistance(xIndexOfMin,yIndexOfMin) + (getActiveSum(xIndexOfMin) - getActiveSum(yIndexOfMin)) / (num_active_nodes - 2)) / 2;
        double yToNewDistance = getDistance(xIndexOfMin, yIndexOfMin) - xToNewDistance;
        int newIndexNode = num_all_nodes;
        int copyOfNewIndex = newIndexNode;
        intToString(copyOfNewIndex);
        // printf("%s\n", "3");
        (nodes + newIndexNode) -> name = *(node_names + newIndexNode);

        *((nodes + newIndexNode)->neighbors) = (nodes + xIndexOfMin);
        *((nodes + newIndexNode)->neighbors + 1) = (nodes + yIndexOfMin);

        for(int i = 0; i < 3; i++){
            if(*((nodes + xIndexOfMin)->neighbors + i) == NULL){
                *((nodes + xIndexOfMin)->neighbors + i) = (nodes + newIndexNode);
                break;
            }
        }

        for(int i = 0; i < 3; i++){
            if(*((nodes + yIndexOfMin)->neighbors + i) == NULL){
                *((nodes + yIndexOfMin)->neighbors + i) = (nodes + newIndexNode);
                break;
            }
        }

        // printf("%s%d\n", "newIndexNode: ", newIndexNode);
        *(*(distances + newIndexNode) + newIndexNode) = 0;
        for(int i = 0; i < num_active_nodes; i++){
            if(*(active_node_map + i) == xIndexOfMin){
                *(*(distances + newIndexNode) + *(active_node_map + i)) = xToNewDistance;
                *(*(distances + *(active_node_map + i)) + newIndexNode) = xToNewDistance;
            }else if(*(active_node_map + i) == yIndexOfMin){
                *(*(distances + newIndexNode) + *(active_node_map + i)) = yToNewDistance;
                *(*(distances + *(active_node_map + i)) + newIndexNode) = yToNewDistance;
            }else{
                *(*(distances + newIndexNode) + *(active_node_map + i)) = 0.5 * (getDistance(xIndexOfMin,*(active_node_map + i)) + getDistance(yIndexOfMin,*(active_node_map + i)) - getDistance(xIndexOfMin,yIndexOfMin));
                *(*(distances + *(active_node_map + i)) + newIndexNode) = *(*(distances + newIndexNode) + *(active_node_map + i));
            }
        }

        fprintf(out, "%d,%d,%.2lf\n", xIndexOfMin,newIndexNode,xToNewDistance);
        fprintf(out, "%d,%d,%.2lf\n", yIndexOfMin,newIndexNode,yToNewDistance);
        // printf("%d,%d,%.2lf\n", xIndexOfMin,newIndexNode,xToNewDistance);
        // printf("%d,%d,%.2lf\n", yIndexOfMin,newIndexNode,yToNewDistance);

        *(active_node_map + findIndexOfMap(xIndexOfMin)) = num_all_nodes;
        *(active_node_map + findIndexOfMap(yIndexOfMin)) = *(active_node_map + (num_active_nodes-1));
        num_all_nodes++;
        num_active_nodes--;

        // globalStatus();
    }
    if(num_active_nodes == 2){
        int aIndex = *(active_node_map);
        int bIndex = *(active_node_map + 1);

        for(int i = 0; i < 3; i++){
            if(*((nodes + aIndex)->neighbors + i) == NULL){
                *((nodes + aIndex)->neighbors + i) = (nodes + bIndex);
                break;
            }
        }
        for(int i = 0; i < 3; i++){
            if(*((nodes + bIndex)->neighbors + i) == NULL){
                *((nodes + bIndex)->neighbors + i) = (nodes + aIndex);
                break;
            }
        }

        fprintf(out, "%d,%d,%.2lf\n", aIndex, bIndex, getDistance(aIndex, bIndex));
        // printf("%d,%d,%.2lf\n", aIndex, bIndex, getDistance(aIndex, bIndex));
    }
    return 0;
}
