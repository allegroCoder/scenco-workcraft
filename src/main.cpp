#if defined(__linux) || defined(__APPLE__)
	#include "config.h"
#else
	#include "D:\Projects\SCENCO\inc\config.h"
#endif

// Tool messages
const char* VERILOG_GENERATED_MESSAGE = "// File generated automatically by ScEnco";
const char* VERILOG_SYSTEM_NAME = "system";

/*GLOBAL VARIABLES*/
char **diff = NULL; 
char **name_cond; 
char **vertices;
char **manual_file;
char **manual_file_back;
char **verilogFiles;
char *numb;
char *file_in = NULL;
char *file_cons = NULL;
char *file_name = NULL;
char **decoder;
char *custom_file_name = NULL;
char *ESPRESSO_PATH;
char *ABC_PATH;
char *LIBRARY_FILE;
char *VERILOG_FILE;
char *SYSTEM_FILE;
char CURRENT_PATH[stringLimit];
char *FOLDER_NAME; 
char **DATAPATH_UNITS;
char **units;
char *buffer;

int **opt_diff = NULL;
int counter = 0;
int **perm = NULL;
int nv=0;
int **cons_perm;
int n_cond = 0;
int *gates,mode = 1;
int tot_enc;
int gen_mode = 2;
int gen_perm = 1000;
int *custom_perm;
int *custom_perm_back;
int mod_bit = 2;
int DATAPATH_N = 2;
int *usage;

//TEMPORARY FILES
#if defined(__linux) || defined(__APPLE__)
	char TRIVIAL_ENCODING_FILE[FILENAME_LENGTH] = "/tmp/trivial.XXXXXX";
	char CONSTRAINTS_FILE[FILENAME_LENGTH] = "/tmp/constraints.XXXXXX";
	char TMP_FILE[FILENAME_LENGTH] = "/tmp/tmpfile.XXXXXX";
	char SCRIPT_PATH[FILENAME_LENGTH] = "/tmp/synth.XXXXXX";
	char BOOL_PATH[FILENAME_LENGTH] = "/tmp/bool.XXXXXX";
	char VERILOG_TMP[FILENAME_LENGTH] = "/tmp/micro.XXXXXX";
#else
	char TRIVIAL_ENCODING_FILE[FILENAME_LENGTH];
	char CONSTRAINTS_FILE[FILENAME_LENGTH];
	char TMP_FILE[FILENAME_LENGTH];
	char SCRIPT_PATH[FILENAME_LENGTH];
	char BOOL_PATH[FILENAME_LENGTH];
	char TMP_NAME[FILENAME_LENGTH];
	char VERILOG_TMP[FILENAME_LENGTH];
#endif

long long int num_perm;

float *area;
long long int *weights = NULL;

CPOG_TYPE **cpog;

boolean unfix = FALSE;
boolean verbose = FALSE;
boolean DC = FALSE;
boolean decode_flag = FALSE;
boolean SET =FALSE;
boolean ABCFLAG = FALSE;
boolean *DC_custom = NULL;
boolean CPOG_SIZE = FALSE;
boolean DISABLE_FUNCTION = FALSE;
boolean OLD = FALSE;
boolean mod_bit_flag = FALSE;
boolean VER = FALSE;
boolean script = FALSE;
boolean DATAPATH = FALSE;
boolean SYSTEM = FALSE;

//ANDREY'S TOOL
GRAPH_TYPE *g;
int n;
char s[stringLimit];

int V;
map<string, int> eventNames;
string eventNames_str[eventsLimit];
map<string, int> eventPredicates[eventsLimit];

vector<string> scenarioNames;
vector<string> scenarioOpcodes;

string ev[eventsLimit][predicatesLimit];
string ee[eventsLimit][eventsLimit];
map<string, vector<pair<int, int> > > constraints;
map<string, vector<pair<int, int> > >::iterator cp, cq;

vector<Encoding> encodings;

vector<string> cgv;
vector<vector<int> > cge;
vector<int> literal;
vector<int> bestLiteral;

string vConditions[eventsLimit][predicatesLimit];
string aConditions[eventsLimit][eventsLimit];

// alternative = false: alpha + beta * predicate
// alternative = true : alpha * (beta + predicate)
bool alternative = false;

int main(int argc, char **argv){

	char *command;

	int bits = 0;
	int j = 0;
	int k=0;
	int err=0;
	int cpog_count = 0;
	int len_sequence = 0;
	int min_bits = 0;
	int *sol;
	int *enc;
	int count_min = 0;
	int count_max = 0;
	int num_vert = 0;
	int elements; 
	int min_disp;
	int mb = 0;

	float ma;
	float MA;
	float mfma;
	float mfMA;
	long long int max = -1;
	long long int min = numeric_limits<long long int>::max();
	long int i;


	struct timeval start;
	struct timeval end;
	struct timeval very_start;
	struct timeval very_end;
	struct timeval begin;
	struct timeval finish;
	struct timeval detail_start;
	struct timeval detail_end;
	long secs_used;
	long precise;
	long total_time;

	gettimeofday(&very_start, NULL);

	FILE *fp = NULL;

	gettimeofday(&begin, NULL);

	/*PARSE PARAMETERS*/
	if( (err = parse_arg(argc, argv)) != 0){
		if (err != 4){
			printError("Error on parsing arguments.");
		}
		return 1;
	}

	// temporary file names
	if (nameTmpFiles() != 0) {
		printError("Error on naming temporary files.");
		removeTempFiles();
		return 1;
	}

	// READ CURRENT PATH POSITION
#if defined(__linux) || defined(__APPLE__)
	if( (fp = popen("pwd","r")) == NULL){
			printError("Error on pwd execution.");
			removeTempFiles();
			return 1;
	}
	if (fgets(CURRENT_PATH,stringLimit,fp) == NULL){
		printError("Error on reading current path.");
		removeTempFiles();
		return 1;
	}
	pclose(fp);

	k = 0;
	for(j = 0; j< ((int)strlen(CURRENT_PATH) - 1); j++){
		if(CURRENT_PATH[j] != '\\')
			CURRENT_PATH[k++] = CURRENT_PATH[j]; 
	}
	CURRENT_PATH[k++] = '/';
	CURRENT_PATH[k] = '\0';

	if (!script) printf("CURRENT: %s\n", CURRENT_PATH);
	fflush(stdout);
#else
	if( (fp = popen("echo %cd%","r")) == NULL){
			printError("Error on pwd execution.");
			removeTempFiles();
			return 1;
	}
	fgets(CURRENT_PATH,stringLimit,fp);
	pclose(fp);
	int lenstr = strlen(CURRENT_PATH);
	CURRENT_PATH[lenstr-1] = '\\';
	k = 0;
#endif

	if (!script) printf("Allocating memory for vertex names and graphs...");
	fflush(stdout);
	name_cond = (char**) malloc(sizeof(char*) * MAX_VERT);
	vertices = (char**) malloc(sizeof(char*) * MAX_VERT);
	g = (GRAPH_TYPE *) malloc(sizeof(GRAPH_TYPE) * scenariosLimit);
	if (!script) printf("DONE\n");
	fflush(stdout);


	// ========================================================================
	// Parsing scenarios
	// ========================================================================

	if (!script) puts("\nOptimal scenarios encoding and CPOG synthesis.\n");	

	if (!alternative){
		if (!script) puts("Using 'f = x + y * predicate' to deal with predicates.\n");
	}
	else{
		if (!script) puts("Using 'f = x * (y + predicate)' to deal with predicates.\n");
	}

	fp = freopen(file_in, "r", stdin);

	n = 0;
	while(scanf("%s", s) == 1)
	{
		if (s[0] == '#')
		{
			if(fgets(s, sizeof(s), stdin) == NULL){
				printError("Error reading scenario.");
				removeTempFiles();
				return 0;
			}
			continue;
		}
		
		if (!strcmp(s, ".scenario"))
		{
			if(scanf("%s", s) == EOF){
				printError("Error reading scenario.");
				removeTempFiles();
				return 0;
			}
			if (!script) printf("Loading scenario '%s'... ", s);
			scenarioNames.push_back(s);
			if (!readScenario()) {
				printError("Error reading scenario.");
				removeTempFiles();
				return 0;
			}
		}
		else
		{
			printError("Wrong file format.");
			removeTempFiles();
			return 0;
		}
	}

	fclose(fp);
	
	if (!script) printf("\n%d scenarios have been loaded.\n", n);
	
	bool predicates_found = false;
	for(int i = 0; i < V; i++)
	if (eventPredicates[i].size() > 0)
	{
		if (!predicates_found)
		{
			predicates_found = true;
			if (!script) puts("\nList of predicates:");
		}
		if (!script) printf("%s:", eventNames_str[i].c_str());
		map<string, int>::iterator p = eventPredicates[i].begin(), q = eventPredicates[i].end();
		while(p != q)
		{
			string pr = p->first;
			if (!script) printf(" %s", pr.c_str());
			p++;
		}
		if (!script) puts("");
	}
	if (!predicates_found) if (!script) puts("\nNo predicates found.");
	
	if( (fp = fopen(CONSTRAINTS_FILE,"w")) == NULL){
		printError("Error on opening constraints file for writing.");
		removeTempFiles();
		return -1;
	}

	for(int i = 0; i < V; i++)
	{
		int np = eventPredicates[i].size();
		
		for(int j = 0; j <= np; j++) ev[i][j] = "";
		if (np == 0)
		{
			for(int j = 0; j < n; j++) if (g[j].v[i]) ev[i][0] += "1"; else ev[i][0] += "0";
			constraints[ev[i][0]].push_back(make_pair(-1, i));
		}
		else
		{
			if (!alternative)
			{	
				for(int j = 0; j < n; j++) if (g[j].v[i] && !g[j].pred[i]) ev[i][0] += "1"; else ev[i][0] += "0";
				constraints[ev[i][0]].push_back(make_pair(-1, i));
				for(int k = 1; k <= np; k++)
				{
					for(int j = 0; j < n; j++)
					if (!g[j].v[i]) ev[i][k] += "0";
					else
					{
						if (g[j].pred[i] == 0) ev[i][k] += "-";
						else
						if (g[j].pred[i] == k) ev[i][k] += "1";
						else
							ev[i][k] += "0";
					}
					constraints[ev[i][k]].push_back(make_pair(-k - 1, i));
				}
			}
			else
			{
				for(int j = 0; j < n; j++) if (g[j].v[i]) ev[i][0] += "1"; else ev[i][0] += "0";
				constraints[ev[i][0]].push_back(make_pair(-1, i));
				for(int k = 1; k <= np; k++)
				{
					for(int j = 0; j < n; j++)
					if (!g[j].v[i]) ev[i][k] += "-";
					else
					{
						if (g[j].pred[i] == 0) ev[i][k] += "1";
						else
						if (g[j].pred[i] == k) ev[i][k] += "0";
						else
							ev[i][k] += "1";
					}
					constraints[ev[i][k]].push_back(make_pair(-k - 1, i));
				}
			}
		}
	}
	
	for(int i = 0; i < V; i++)
	for(int j = 0; j < V; j++)
	if (i != j)
	{
		ee[i][j] = "";
		for(int k = 0; k < n; k++)
		{
			if (g[k].e[i][j] == 2 || ME(k, i, j)) ee[i][j] += "-";
			else
			if (g[k].e[i][j] == 1) ee[i][j] += "1";
			else ee[i][j] += "0";
		}
		constraints[ee[i][j]].push_back(make_pair(i, j));
	}
	
	cp = constraints.begin(); cq = constraints.end();
	while(cp != cq)
	{
		string s = cp->first;
		fprintf(fp,"%s       ", s.c_str());
		int k = cp->second.size();
		for(int i = 0; i < k; i++)
		{
			int a = cp->second[i].first;
			int b = cp->second[i].second;
			if (a < 0)
			{
				if (a == -1) fprintf(fp," %s", eventNames_str[b].c_str());
				else fprintf(fp," %s:%s", eventNames_str[b].c_str(), getPredicateName(b, -a - 1).c_str());
			}
			else
			{
				fprintf(fp," (%s -> %s)", eventNames_str[a].c_str(), eventNames_str[b].c_str());
			}
		}
		fprintf(fp,"\n");
		cp++;
		
		Encoding e;
		
		e.constraint = s;
		e.trivial = true;
		e.constant = 0;
		
		for(int i = 0; i < n; i++) if (s[i] == '1') { e.trivial = false; break;}

		if (!e.trivial)
		{
			e.trivial = true;
			e.constant = 1;
			
			for(int i = 0; i < n; i++) if (s[i] == '0') { e.trivial = false; break;}
		}
		
		encodings.push_back(e);
	}
	fclose(fp);
	int total = constraints.size(), trivial = 0;

	
	for(int i = 0; i < total; i++) if (encodings[i].trivial) trivial++;

	if (!script) printf("\n%d non-trivial encoding constraints found:\n\n", total - trivial);
	
	if( (fp = fopen(TRIVIAL_ENCODING_FILE,"w")) == NULL){
		printError("Error on opening constraints file for writing.");
		removeTempFiles();
		return -1;
	}
	for(int i = 0; i < total; i++)
		if (!encodings[i].trivial) {
			fprintf(fp,"%s\n",encodings[i].constraint.c_str());
	}
	fclose(fp);
	
	if (!script) printf("\nBuilding conflict graph... ");
	
	for(int i = 0; i < total; i++)
	if (!encodings[i].trivial)
	{
		string s = encodings[i].constraint;
		cgv.push_back(s);
		for(int j = 0; j < n; j++) if (s[j] == '0') s[j] = '1'; else if (s[j] == '1') s[j] = '0';
		cgv.push_back(s);
	}
	
	cge.resize(cgv.size());
	literal.resize(cgv.size());
	bestLiteral.resize(cgv.size());
	for(unsigned int i = 0; i < cgv.size(); i += 2) { bestLiteral[i] = i / 2; bestLiteral[i + 1] = -1;}
	
	for(unsigned int i = 0; i < cgv.size(); i++)
	for(unsigned int j = 0; j < cgv.size(); j++)
	{
		string a = cgv[i];
		string b = cgv[j];
		
		bool conflict = false;
		
		for(int k = 0; k < n; k++)
			if ((a[k] == '0' && b[k] == '1') || (a[k] == '1' && b[k] == '0'))
			{
				conflict = true;
				break;
			}
		
		if (conflict) cge[i].push_back(1); else cge[i].push_back(0);
	}
	
	if (!script) printf("DONE.\n");
	fflush(stdout);

	// single literal encoding
	if (OLD){
		if (!script) printf("Single literal encoding... ");
		fflush(stdout);
		int L = 0, R = cgv.size() / 2, cnt = 1;
		while(R - L > 1)
		{
			int limit = (L + R) / 2;
		
			for(unsigned int i = 0; i < cgv.size(); i++) literal[i] = -1;
		
			if (!script) printf(" [%d]", cnt++);
		
			bool res = false;
			res = encode(0, limit, 0);
		
			if (res)
			{
				bestLiteral = literal;
				R = limit;
			}
			else L = limit;
		}
	
		if (!script) printf("DONE.\nThe best encoding uses %d operational variables:\n", R);
		fflush(stdout);
	
		scenarioOpcodes.resize(n);

		for(int i = 0; i < n; i++) for(int j = 0; j < R; j++) scenarioOpcodes[i] += "-";
	    
		k = 0;
		for(int i = 0; i < total; i++)
		if (!encodings[i].trivial)
		{
			int id = k * 2, inv = 0;
		
			if (bestLiteral[id] == -1) inv = 1;
		
			if (!script) printf("%s        ", cgv[id].c_str());
			if (inv) if (!script) printf("!");
			if (!script) printf("x[%d]\n", bestLiteral[id + inv]);
		
			encodings[i].literal = bestLiteral[id + inv];
			encodings[i].inverted = inv;
		
			for(int j = 0; j < n; j++) if (cgv[id][j] != '-') scenarioOpcodes[j][bestLiteral[id + inv]] = cgv[id + inv][j];

			k++;
		}
	
		for(int i = 0; i < total; i++)
		{
			string s = encodings[i].constraint;
			char tmp[10];
		
			string f = "";
			if (encodings[i].trivial)
			{
				f += '0' + encodings[i].constant;
			}
			else
			{
				sprintf(tmp, "x[%d]", encodings[i].literal);
				f = tmp;
				if (encodings[i].inverted) f = "!" + f;		
			}		
		
			for(unsigned int j = 0; j < constraints[s].size(); j++)
			{
				int a = constraints[s][j].first;
				int b = constraints[s][j].second;
			
				if (a < 0) vConditions[b][-a - 1] = f;
				else aConditions[a][b] = f;
			}
		}
	
		if (!script) puts("\nVertex conditions:\n");
	
		for(int i = 0; i < V; i++)
		{
			string f = vConditions[i][0];
			map<string, int>::iterator p = eventPredicates[i].begin(), q = eventPredicates[i].end();
		
			int k = 1;
			while(p != q)
			{
				if (!alternative)
				{
					if (vConditions[i][k] != "1") f += " + " + vConditions[i][k] + " * " + (p->first);
					else f += " + " + (p->first);
					p++;
					k++;
				}
				else
				{
					if (vConditions[i][k] != "0") f += " * (" + vConditions[i][k] + " + " + (p->first) + ")";
					else f += " * " + (p->first);
					p++;
					k++;
				}
			}
			if (f.find("0 + ") == 0) f.erase(0, 4);
			if (f.find("1 * ") == 0) f.erase(0, 4);
			if (!script) printf("%10s: %s\n", eventNames_str[i].c_str(), f.c_str());
		}

		if (!script) puts("\nArc conditions:\n");
	
		for(int i = 0; i < V; i++)
		for(int j = 0; j < V; j++)
		if (i != j)
		{
			string f = aConditions[i][j];
			if (f == "0") continue;

			if (!script) printf("%10s -> %-10s: %s\n", eventNames_str[i].c_str(), eventNames_str[j].c_str(), f.c_str());
		}

		if (!script) printf("\n.scen_opcodes \n");

		for(int i = 0; i < n; i++) if (!script) printf("%s\n",scenarioOpcodes[i].c_str());
		if (!script) printf(".end_scen_opcodes \n");
		mb = scenarioOpcodes[0].length();
	}

	if (!script) printf("Free memory related to graphs acquisition...");
	fflush(stdout);
	free(g);
	if (!script) printf("DONE\n");
	fflush(stdout);

	// ========================================================================
	// Reading non-trivial encodings
	// ========================================================================
	if (!script) printf("\n================================================\n");
	if (!script) printf(" Reading non-trivial encodings\n");
	if (!script) printf("================================================\n\n");
	fflush(stdout);

	// READ NON-TRIVIAL ENCODING FILE
	if (!script) printf("Reading non-trivial encoding file... ");
	fflush(stdout);
	if( (err = read_file(TRIVIAL_ENCODING_FILE, &cpog_count, &len_sequence)) ){
		printErrorCode("Error while reading non-trivial encoding file", err);
		removeTempFiles();
		return 2;
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	// SEED FOR RAND
	srand(time(NULL));

	// ALLOCATING AND ZEROING DIFFERENCE MATRIX
	opt_diff = (int**) calloc(cpog_count, sizeof(int*));
	for(i=0;i<cpog_count;i++)
		opt_diff[i] = (int*) calloc(cpog_count, sizeof(int));

	// COMPUTING LOGARITHM OF NUMBER OF CPOGs
	if(mod_bit_flag){
		bits = mod_bit;
	}else{
		bits = logarithm2(cpog_count);
	}

	// NUMBER OF POSSIBLE ENCODING
	tot_enc = 1;
	for(i=0;i<bits;i++) tot_enc *= 2;
	if(mod_bit_flag){
		if (!script) printf("Custom number of bits set to encode all CPOG is: %d\n",bits);		
	} else{
		if (!script) printf("Miminum number of bits needed to encode all CPOG is: %d\n",bits);
	}

	/// ANALYSIS IF IT'S A PERMUTATION OR A DISPOSITION
	num_perm = 1;
	if (cpog_count == tot_enc){
		// permuation
		if(!unfix && !SET){
			for(i = 1; i< tot_enc; i++)
				num_perm *= i;
		}else{
			for(i = 1; i<= tot_enc; i++)
				num_perm *= i;
		}
		if (!script) printf("Number of possible permutations by fixing first element: %lld\n", num_perm);
	}
	else{
		// DISPOSITION
		if(!unfix && !SET){
			elements = tot_enc-1;
			min_disp = elements - (cpog_count - 1) + 1;
		}else{
			elements = tot_enc;
			min_disp = elements - (cpog_count) + 1;
		}
			num_perm = 1;
		for(i=elements; i>= min_disp; i--)
			num_perm *= i;
		if (!script) printf("Number of possible dispositions by fixing first element: %lld\n", num_perm);
	}

	if(gen_mode > 1){
		if(num_perm > MAX_MEMORY || num_perm < 0){
			printError("Solution space is too wide to be inspected.");
			removeTempFiles();
			return 3;
		}
	}else{
		num_perm = gen_perm;
	}

	// PREPARATION DATA FOR ENCODING PERMUTATIONS
	enc = (int*) calloc(tot_enc, sizeof(int));

	// First element is fixed
	if (!unfix && !SET)
		enc[0] = 1;
	
	sol = (int*) calloc(tot_enc, sizeof(int));
	if (sol == NULL){
		printError("Failed memory allocation for 'sol' variable.");
		removeTempFiles();
		return 3;
	}
	perm = (int**) malloc(sizeof(int*) * num_perm);
	if ( perm == NULL){
		printError("Failed memory allocation for 'perm' variable.");
		removeTempFiles();
		return 3;
	}
	for(i=0;i<num_perm;i++){
		perm[i] = (int*) malloc(cpog_count * sizeof(int));
		if (perm[i] == NULL){
			printf(".error \n");
			printf("Failed memory allocation for 'perm[%ld]'\n", i);
			printf(".end_error \n");
			removeTempFiles();
			return 3;
		}
	}
	weights = (long long int*) calloc(num_perm, sizeof(long long int));


	/*BUILDING DIFFERENCE MATRIX*/
	if (!script) printf("Building DM (=Difference Matrix)... ");
	fflush(stdout);
	if( (err = difference_matrix(cpog_count, len_sequence)) ){
		printErrorCode("Error occurred while building difference matrix", err);
		removeTempFiles();
		return 3;
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	// ========================================================================
	// Building CPOG
	// ========================================================================
	if (!script) printf("\n================================================\n");
	if (!script) printf(" Building CPOG\n");
	if (!script) printf("================================================\n\n");
	fflush(stdout);

	/*FIRST READING OF ENCODING FILE*/
	if (!script) printf("First reading of encoding file...");
	fflush(stdout);
	if( (err = read_cons(CONSTRAINTS_FILE, cpog_count, &num_vert)) ){
		printErrorCode("Error occured while reading constraints file", err);
		removeTempFiles();
		return 5;
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	// CPOG ALLOCATION
	if (!script) printf("CPOG data structure allocation...");
	fflush(stdout);
	cpog = (CPOG_TYPE**) malloc(sizeof(CPOG_TYPE*) * (num_vert));
	for(i=0;i<num_vert; i++)
		cpog[i] = (CPOG_TYPE*) malloc(sizeof(CPOG_TYPE) * (num_vert));
	if (!script) printf("DONE\n");
	fflush(stdout);

	nv = num_vert;

	// CPOG DEFINITION
	for(i=0;i<num_vert; i++)
		for(j=0;j<num_vert;j++)
			if(i == j){
				cpog[i][j].type = 'v';
				cpog[i][j].source = strdup(vertices[i]);
				cpog[i][j].dest = strdup("X");
				cpog[i][j].condition = FALSE;
			}
			else{
				cpog[i][j].type = 'e';
				cpog[i][j].source = strdup(vertices[i]);
				cpog[i][j].dest = strdup(vertices[j]);
				cpog[i][j].condition = FALSE;
			}

	// SECOND READING OF ENCODING FILE
	if (!script) printf("Second reading of encoding file (parsing CPOG)...");
	fflush(stdout);
	parsing_cpog(CONSTRAINTS_FILE, cpog_count, num_vert);
	if (!script) printf("DONE\n");
	fflush(stdout);

	if (!script) printf("CPOG read properly.\n");

	// ========================================================================
	// Generating encoding
	// ========================================================================
	if (!script) printf("\n================================================\n");
	if (!script) printf(" Generating encodings\n");
	if (!script) printf("================================================\n\n");
	fflush(stdout);

	// START COUNTING TIME
	gettimeofday(&start, NULL);

	if(OLD){
		if (!script) printf("Reading encodings set (OLD flag)... ");
		fflush(stdout);
		if( (fp = fopen(custom_file_name,"w")) == NULL ){
			printError("Error on opening custom file.");
			removeTempFiles();
			return 2;
		}
		for(i=0; i<cpog_count; i++) fprintf(fp,"%s\n",scenarioOpcodes[i].c_str());
		int nbits = strlen(scenarioOpcodes[0].c_str());
		fprintf(fp, "%d", nbits);
		fclose(fp);
		if (!script) printf("DONE\n");
		fflush(stdout);
	}

	if(SET){
		if (!script) printf("Reading encodings set... ");
		fflush(stdout);
		if(read_set_encoding(cpog_count,&bits) != 0){
			printError("Error on reading encoding set.");
			removeTempFiles();
			return 1;
		}
		if (!script) printf("DONE\n");
		if (!script) printf("Check correcteness of encoding set... ");
		fflush(stdout);
		if(check_correctness(cpog_count,tot_enc,bits) != 0){
			removeTempFiles();
			return 1;
		}
		if (!script) printf("DONE\n");
		fflush(stdout);
	}

	// ENCODING GENERATION
	switch(gen_mode){

		// random search
		case 0:
			if (!script) printf("Selected random encoding generation. %lld encodings will be generated.\n", num_perm);
			if (!SET){
				if (!script) printf("Random algorithm generation unconstrained... ");
				fflush(stdout);
				rand_permutation(cpog_count, tot_enc);
				if (!script) printf("DONE\n");
				fflush(stdout);
			}
			else{
				if (!script) printf("Random algorithm generation constrained... ");
				fflush(stdout);
				rand_permutations_constraints_v2(cpog_count,tot_enc,bits);
				if (!script) printf("DONE\n");
				fflush(stdout);
			}

			break;

		// heuristic encoding
		case 1:
			if (!script) printf("Selected herustic encoding generation. %lld encodings will be generated.\n", num_perm);

			gettimeofday(&detail_start, NULL);
			if (!SET){
				if (!script) printf("Starting encoding generation unconstrained... ");
				fflush(stdout);
				rand_permutation(cpog_count, tot_enc);
				if (!script) printf("DONE\n");
				fflush(stdout);
			}
			else{
				if (!script) printf("Starting encoding generation constrained... ");
				fflush(stdout);
				rand_permutations_constraints_v2(cpog_count,tot_enc,bits);
				if (!script) printf("DONE\n");
				fflush(stdout);
			}
			gettimeofday(&detail_end, NULL);
			precise=(detail_end.tv_sec - detail_start.tv_sec);
			if (!script) printf("Time for first vectors generation: %ld [s]\n", precise);
			fflush(stdout);

			gettimeofday(&detail_start, NULL);
			if (!script) printf("Tune vector by using simulated annealing... ");
			fflush(stdout);
			start_simulated_annealing(cpog_count,tot_enc,bits);
			if (!script) printf("DONE\n");
			fflush(stdout);
			gettimeofday(&detail_end, NULL);
			precise=(detail_end.tv_sec - detail_start.tv_sec);
			if (!script) printf("Time for tuning vectors: %ld [s]\n", precise);
			fflush(stdout);
			break;

		default:
			if(!unfix && !SET){
				//permutation_stdalgo(cpog_count,tot_enc);
				if (!script) printf("Permutation algorithm unconstrained.\n");
				fflush(stdout);
				permutation(sol,0,enc,cpog_count, tot_enc);
				if (!script) printf("DONE\n");
				fflush(stdout);
			}else{
				if (!script) printf("Permutation algorithm constrained.\n");
				fflush(stdout);
				permutation(sol,-1,enc,cpog_count, tot_enc);
				if (!script) printf("DONE\n");

				if (!script) printf("Filtering encoding... ");
				fflush(stdout);
				filter_encodings(cpog_count, bits, tot_enc);
				if (!script) printf("DONE\n");
				fflush(stdout);
			}

			break;
	}

	// TIME SPENT FOR GENERATING ENCODINGS
	gettimeofday(&end, NULL);
	secs_used=(end.tv_sec - start.tv_sec);
	
	/*COMPUTING WEIGHT FOR EACH ENCODING*/
	max = -1;
	min = area_encodings_ssd(cpog_count, bits, &max,tot_enc,num_vert);

	if (!script) printf("Maximum weight for all possible permutations: %lld\n", max);
	if (!script) printf("Minimum weight for all possible permutations: %lld\n", min);

	/*COUNTING HOW MANY ENCODINGS HAVE MAX WEIGHT AND MAX AREA*/
	for(i=0; i<counter;i++){
		if(max == weights[i]){
			count_max++;
		}
	}

	if (!script) printf("Number of encodings with maximum weight: %d\n", count_max);

	/*COUNTING HOW MANY ENCODINGS HAVE MIN WEIGHT AND MIN AREA*/
	for(i=0; i<counter;i++){
		if(min == weights[i]){
			count_min++;
		}
	}

	if (!script) printf("Number of encodings with minimum weight: %d\n", count_min);

	// Memory allocation for synthesis
	if (!script) printf("Memory allocation for synthesis... ");
	fflush(stdout);
	if(manage_data_base(count_min,min,count_max,max,cpog_count,&bits)){
		printError("Error on allocating memory for synthesis.");
		removeTempFiles();
		return 4;
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	// FREE VARIABLES NO MORE USEFUL
	if (!script) printf("Free memory of the variables no more used... ");
	fflush(stdout);
	free(enc);
	free(sol);
	for(i=0;i<cpog_count;i++)
		free(opt_diff[i]);
	free(opt_diff);
	for(i=0;i<len_sequence;i++)
		free(diff[i]);
	free(diff);
	free(file_in);
	if (!script) printf("DONE\n");
	fflush(stdout);

	if (!script) printf("Time takes for generating encodings: %ld [s].\n",secs_used);
	fflush(stdout);

	// ========================================================================
	// Extracting Boolean equations from the CPOG
	// ========================================================================
	if (!script) printf("\n================================================\n");
	if (!script) printf(" Extracting Boolean equations from the CPOG\n");
	if (!script) printf("================================================\n\n");
	fflush(stdout);

	if (!script) printf("Memory allocation phase... ");	
	fflush(stdout);
	// ALLOCATION MEMORY FOR ALL LOGIC FUNCTIONS CONSIDERED
	for(i=0;i<nv; i++){
		for(j=0;j<nv;j++){
			cpog[i][j].fun = (char**) malloc(sizeof(char*) * counter);
			cpog[i][j].fun_cond = (char**) malloc(sizeof(char*) * counter);
		}
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	// START COUNTING TIME
	gettimeofday(&start, NULL);

	//CONVERT TRUTH TABLES INTO BOOLEAN FUNCTION
	if (!script) printf("Convert truth table into boolean functions of vertices and edges... ");
	fflush(stdout);
	if((err = boolean_function(max,bits,cpog_count,0)!= 0)){
		printErrorCode("Error on getting boolean function using Espresso", err);
		removeTempFiles();
		return 6;
	}
	if (!script) printf("DONE\n");
	fflush(stdout);


	// CONVERT TRUTH TABLES INTO BOOLEAN FUNCTION OF CONDITION ONLY
	if (!script) printf("Convert truth table into boolean functions of condition of vertices... ");
	fflush(stdout);
	if((err = boolean_function(max,bits,cpog_count,1)!= 0)){
		printErrorCode("Error on getting boolean function using Espresso", err);
		removeTempFiles();
		return 7;
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	min_bits = logarithm2(cpog_count);
	if(min_bits < bits && mode ==3){
		if (!script) printf("Convert truth table into boolean functions (decoder minimisation)... ");
		fflush(stdout);
		decode_flag = TRUE;
		if(decoder_minimisation(bits,cpog_count) != 0){
			printError("Error on developing boolean functions of decoder.");
			removeTempFiles();
			return 8;
		}
		if (!script) printf("DONE\n");
		fflush(stdout);
	}

	gettimeofday(&end, NULL);
	secs_used=(end.tv_sec - start.tv_sec);
	if (!script) printf("Conversion done successfully.\n");
	if (!script) printf("Time of conversion= %ld [s].\n",secs_used);

	// ========================================================================
	// Synthesis of hardware implementation (tech. mapping if ABC is present)
	// ========================================================================
	if (!script) printf("\n================================================\n");
	if (!script) printf(" Synthesis of hardware implementation (tech. mapping if ABC is present)\n");
	if (!script) printf("================================================\n\n");
	fflush(stdout);

	// REMOVE OLD DIR AND MAKE NEW ONE AND ERASE USED FILES
#if defined(__linux) || defined(__APPLE__)
	command = strdup("rm -f ");
#else
	command = strdup("del ");
#endif
	command = catMem(command, TRIVIAL_ENCODING_FILE);
	if(system(command) != 0){
		printError("Error on removing trivial encoding file.");
		removeTempFiles();
		return 8;
	}
	free(command);

#if defined(__linux) || defined(__APPLE__)
	command = strdup("rm -f ");
#else
	command = strdup("del ");
#endif
	command = catMem(command, CONSTRAINTS_FILE);
	if(system(command) != 0){
		printError("Error on removing trivial encoding file.");
		removeTempFiles();
		return 8;
	}
	free(command);
	

	// ACQUIRE NAMES OF CONDITIONS
	if (!script) printf("Getting condition names... ");
	fflush(stdout);
	if(get_conditions_names()){
		printError("Error on getting condition names from CPOG representation.");
		removeTempFiles();
		return 8;
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	// START COUNTING TIME
	gettimeofday(&start, NULL);

	if (!script) printf("Building final Boolean equations (uses ABC if selected)... ");
	fflush(stdout);
	if(!CPOG_SIZE){
		if( (err = equations_abc(cpog_count,bits)) != 0){
			printErrorCode("Error on using ABC", err);
			removeTempFiles();
			return 9;
		}
	}else{
		if( (err = equations_abc_cpog_size(cpog_count,bits)) != 0){
			printErrorCode("Error on using ABC", err);
			removeTempFiles();
			return 9;
		}
	}
	if (!script) printf("DONE\n");
	fflush(stdout);

	// COMPUTES TIME SPENT BY FUNCTION equations_abc
	gettimeofday(&end, NULL);
	gettimeofday(&finish, NULL);
	secs_used=(end.tv_sec - start.tv_sec);
	total_time=(finish.tv_sec - begin.tv_sec);
	if (!script) printf("Reduction and mapping done successfully.\n");
	if (!script) printf("Time spent= %ld [s].\n\n",secs_used);
	if (!script) printf("Total SCENCO run time: %ld [s].\n", total_time);
	fflush(stdout);

	// ========================================================================
	// Statistics generation for Workcraft
	// ========================================================================
	if (!script) printf("\n================================================\n");
	if (!script) printf(" Statistics generation for Workcraft\n");
	if (!script) printf("================================================\n\n");
	fflush(stdout);

	// IF ABC TOOL IS NOT PRESENT OUTPUT A RANDOM ENCODING
	if(ABCFLAG == FALSE){
		i = rand() % counter;

		if (!script) printf("MIN: ");
		for(k=0;k<cpog_count;k++){
			if(SET && DC_custom[k]){
				char str[MAX_NAME];
				int_to_string_DC(bits, k,cons_perm[i][k], str);
				if (!script) printf("%s ",str);
			}
			else{
				if (!script) print_binary(stdout,cons_perm[i][k], bits);
			}

		}
		if (!script) printf("\n");
		
		if (!script) printf(".start_formulae \n");
		for(j=0;j<nv; j++){
			for(k=0;k<nv;k++){
				if(cpog[j][k].type == 'v'){
					if(cpog[j][k].condition){
						if (!script) printf("V,%s,%s,(%s) + ((%s) * (%s))\n",cpog[j][k].source,cpog[j][k].truth,cpog[j][k].fun[i], cpog[j][k].cond,cpog[j][k].fun_cond[i]);
					}
					else{
						if (!script) printf("V,%s,%s,(%s)\n",cpog[j][k].source,cpog[j][k].truth,cpog[j][k].fun[i]);
					}
				}
				else{
					if(!decide(cpog[j][k].fun[i]))
						if (!script) printf("A,%s,%s,%s,(%s)\n",cpog[j][k].source, cpog[j][k].dest,cpog[j][k].truth,cpog[j][k].fun[i]);
				}
			}
		}
		if (!script) printf(".end_formulae \n");


		if (!script) printf(".statistics \n");
		if (!script) printf("Boolean equations for each element:\n");
		for(j=0;j<nv; j++){
			for(k=0;k<nv;k++){
                if ( strcmp(cpog[j][k].source, "GO") != 0 && strcmp(cpog[j][k].source, "DONE") != 0 ) {
				    if(cpog[j][k].type == 'v'){
					    if(cpog[j][k].condition){
						    if (!script) printf("%s = (%s) + ((%s) * (%s))\n",cpog[j][k].source,cpog[j][k].fun[i], cpog[j][k].cond,cpog[j][k].fun_cond[i]);
					    }
					    else{
						    if (!script) printf("%s = (%s)\n",cpog[j][k].source,cpog[j][k].fun[i]);
					    }
				    }
				    else{
					    if(!decide(cpog[j][k].fun[i]))
						    if (!script) printf("%s -> %s = (%s)\n",cpog[j][k].source, cpog[j][k].dest,cpog[j][k].fun[i]);
				    }
                }
			}
		}
		if (!script) printf("\n");
		if (!script) printf(".end_statistics \n");
		removeTempFiles();
		return 0;
	}

	ma = numeric_limits<float>::max();
	MA = -1;
	mfma = numeric_limits<float>::max();
	mfMA = -1;
	for(i=0;i<counter;i++){
		/*COUNT MAX ERROR AND MIN ERROR*/		
		if(mode == 2){
			if(area[i] < ma) ma = area[i];
			if(area[i] > MA) MA = area[i];
			if(weights[i] == min && area[i] < mfma) mfma = area[i];
			if(weights[i] == min && area[i] > mfMA) mfMA = area[i];
		}
		else{
			if(area[i] < ma) ma = area[i];
			if(area[i] > MA) MA = area[i];
		}
	}
	j = 0;

	/*PRINTING BEST ENCODINGS FOUND*/
	if(mode != 3){
		if(mode == 2){
			if (!script) printf("RESULTS:\n");
		}
		float i_avg=0;
		float avg = 0.0;
		if(mode == 2){
			if (!script) printf("ALL ENCODINGS CHARACTERISTICS:\n");
			for(i=0;i<counter;i++){
				for(k=0;k<cpog_count;k++){
					if(SET && DC_custom[k]){
						char str[MAX_NAME];
						int_to_string_DC(bits, k,cons_perm[i][k], str);
						if (!script) printf("(%d) %s ",cons_perm[i][k],str);
					}
					else{
						if (!script) print_binary(stdout,cons_perm[i][k], bits);
					}
				}
				if (!script) printf("with %d gates and %.3f [um^2] and weight %lld.\n", gates[i],area[i],weights[i]);
				
			}
			if (!script) printf("-----------------------\n\n");
		}

		gettimeofday(&very_end, NULL);
		secs_used=(very_end.tv_sec - very_start.tv_sec);

		for(i=0;i<counter;i++){
			if(area[i] == ma){
				if (!script) printf(".area \n");
				if (!script)printf("%.3f\n", area[i]);
				else{
					if (!OLD) mb = min_bits;
					else mb = mb + n_cond;
					if (secs_used == 0) secs_used++;
					printf("%.3f %d %d %ld\n", area[i],gates[i], mb, secs_used);
				}
				if (!script) printf(".end_area \n\n");
				if (!script) printf("MIN: ");
				for(k=0;k<cpog_count;k++){
					if(SET && DC_custom[k]){
						char str[MAX_NAME];
						int_to_string_DC(bits, k,cons_perm[i][k], str);
						if (!script) printf("%s ",str);
					}
					else{
						if (!script) print_binary(stdout,cons_perm[i][k], bits);
					}
				}
				if (!script) printf("\n");
				if (!script) printf(".start_formulae \n");
				for(j=0;j<nv; j++){
					for(k=0;k<nv;k++){
						if(cpog[j][k].type == 'v'){
							if(cpog[j][k].condition){
								if (!script) printf("V,%s,%s,(%s) + ((%s) * (%s))\n",cpog[j][k].source,cpog[j][k].truth,cpog[j][k].fun[i], cpog[j][k].cond,cpog[j][k].fun_cond[i]);
							}
							else{
								if (!script) printf("V,%s,%s,(%s)\n",cpog[j][k].source,cpog[j][k].truth,cpog[j][k].fun[i]);
							}
						}
						else{
							if(!decide(cpog[j][k].fun[i]))
								if (!script) printf("A,%s,%s,%s,(%s)\n",cpog[j][k].source, cpog[j][k].dest,cpog[j][k].truth,cpog[j][k].fun[i]);
						}
					}
				}
				if (!script) printf(".end_formulae \n");
				if (!script) printf(".statistics \n");
				if (!script) printf("Boolean equations for each element:\n");
				for(j=0;j<nv; j++){
					for(k=0;k<nv;k++){
                        if ( strcmp(cpog[j][k].source, "GO") != 0 && strcmp(cpog[j][k].source, "DONE") != 0 ) {
						    if(cpog[j][k].type == 'v'){
							    if(cpog[j][k].condition){
								    if (!script) printf("%s = (%s) + ((%s) * (%s))\n",cpog[j][k].source,cpog[j][k].fun[i], cpog[j][k].cond,cpog[j][k].fun_cond[i]);
							    }
							    else{
								    if (!script) printf("%s = (%s)\n",cpog[j][k].source,cpog[j][k].fun[i]);
							    }
						    }
						    else{
							    if(!decide(cpog[j][k].fun[i]))
								    if (!script) printf("%s -> %s = (%s)\n",cpog[j][k].source, cpog[j][k].dest,cpog[j][k].fun[i]);
						    }
                        }
					}
				}
				if (!script) printf("\n");
				if (!script) printf("Area and number of gates achieved with library set:\n");
				if (!script) printf("Controller for CPOG consumes %.3f [um^2] and %d gates are needed.\n", area[i],gates[i]);
				if (!script) printf(".end_statistics \n");

				if (!CPOG_SIZE){
					useABC(verilogFiles[i]);
					if (VER) replaceVerilogName();

					if (SYSTEM) {
						if (!script) printf("Synthesis of the interface started... ");
						if (buildInterface(bits) != 0){
							printError("Error on parsing datapath units.");
							removeTempFiles();
						}
						if (!script) printf("DONE\n");
					}
				}

				removeTempFiles();
				return 0;
			}
		}
		if(mode == 2){
			if (!script) printf("Maximum error = %.2f.\n", ((mfMA/ma) -1) *100);
			if (!script) printf("Minimum error = %.2f.\n", ((mfma/ma) -1) *100);
			if (!script) printf("Average error = %.2f.\n", ( ((avg/i_avg)/ma) -1) *100);
		}
		else
			if (!script) printf("Overhead at the minimum point = %.2f.\n", ((MA/ma) -1) *100);
	}
	else{
		if (!script) printf("Encoding set comes up with: %d gates and %.3f [um^2].\n",gates[0], area[0]);
	}

	removeTempFiles();
	
	if (!script) printf("Free memory for vertex names... ");
	fflush(stdout);
	free (name_cond);
	free(vertices);
	if (!script) printf("DONE");
	fflush(stdout);

	return 0;
}
