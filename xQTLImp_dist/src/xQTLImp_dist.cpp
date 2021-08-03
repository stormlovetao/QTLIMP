#include <thread>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include "impute.h"
#include "load_info.h"
#include "comp.h"
#include "utils.h"
#define MAX_THREAD_NUM 100

struct thread_data{
   map<string,long long int>* p_VcfIndex;
   vector<string>* p_VcfFile;
   map<string,long long int*> *pos_map;
   int chrom;
   string Xqtl_path;
   string ref_file;
   string out_dir;
   long long int start;
   long long int end;
  double maf;
  double lam;
  double delta;
  int window_size;
};

void *main_process(thread_data *threadarg , int num)
{
	struct thread_data *my_data;
	my_data = threadarg;
   	map<string,long long int>* p_VcfIndex = my_data -> p_VcfIndex;
   	vector<string>* p_VcfFile = my_data -> p_VcfFile;
   	map<string,long long int*> *pos_map = my_data -> pos_map;

   	string ref_file = my_data -> ref_file;
   	string Xqtl_path = my_data -> Xqtl_path;
   	string out_dir = my_data -> out_dir;
   	long long int start = my_data -> start;
   	long long int end = my_data -> end;
	int window_size = my_data -> window_size;
   	int chrom = my_data -> chrom;
	double maf = my_data -> maf;
	double lam  = my_data -> lam;
	double delta = my_data -> delta;

	ifstream fin(Xqtl_path.c_str());
	fin.seekg(start);
	string line;
	vector<snps> origin_typed_snps;
	vector<snps> *p_origin_typed_snps = &origin_typed_snps;
	vector<typed_snp> typed_snps;
	vector<typed_snp> *p_typed_snps = &typed_snps;
	vector<typed_snp> maf_snps;
	vector<typed_snp> *p_maf_snps = &maf_snps;
	vector<snps> ignore_snps;
	vector<snps> *p_ignore_snps = &ignore_snps;
	vector<ref_snp> snp_map;
	vector<ref_snp> *p_snp_map = &snp_map;

	vector<string> hap;
	vector<string> *p_hap = &hap;

	//start extract
	getline(fin,line);
	string line_list[100];
	split_line(line_list,line);
	string name = line_list[1];
	string last_name;
	record_all(p_origin_typed_snps , line_list[2] , line_list[3] , line_list[4] , line_list[5]);
	//record the molecular

	map<long long int , int> m_all_snps ;
	map<long long int , int> m_typed_snps ;
	map<long long int , int> *p_m_all_snps = &m_all_snps;
	map<long long int , int> *p_m_typed_snps = &m_typed_snps;
		ans values;
		values.last_sigma_it = NULL;
		values.weight = NULL;
		values.row = 0;
		int flag = 0;
		while(fin.tellg() <= end)
		{
			last_name = name;
			getline(fin,line);
			if(line == "")
			{
				flag = 1;
			}
			init_line_list(line_list);
			split_line(line_list,line);
			name = line_list[1];

			if(last_name != name || 1 == flag)
			{
			//calculate window
				long long int window[2];
				calculate_window(window_size , window , last_name , pos_map);

			// 1.split origin_typed_snps into typed_snps
			//and unuseful snps(wrong and special type)
				filter_snps(ref_file , last_name,p_origin_typed_snps ,
						 p_typed_snps , p_ignore_snps,window , p_VcfIndex,p_VcfFile);
			//2.search  annotation map and pos file dict to generate
			//.map and .hap vector with .vcf
				gen_map_hap(ref_file , last_name , p_snp_map , p_hap , window,p_VcfIndex,p_VcfFile);
			//3.impute process
				size_t num_typed_snps = typed_snps.size();
				size_t num_total_snps = snp_map.size();
				vector<char> convert_flags;
				convert_flags.resize(num_typed_snps, 0);
				vector<char> impute_flags;
				impute_flags.resize(num_total_snps, 1);
				mark_snps(typed_snps, snp_map, convert_flags, impute_flags);
    			// convert z-score
				for(size_t i = 0; i < num_typed_snps; i++)
				{
					if(convert_flags[i] == 1)
					{
						typed_snps[i].zscore *= -1.0;
					}
				}
				vector<long long int> useful_typed_snps;
				vector<long long int>* p_useful_typed_snps = &useful_typed_snps;
				vector<int> snps_flag;
				vector<int>* p_snps_flag = &snps_flag;
				// TSS of gene.
				long long int start_pos = (*pos_map)[last_name][0];
				values = zgenbt(p_maf_snps , maf , lam , p_snps_flag ,typed_snps, snp_map , convert_flags , impute_flags ,
                                hap , p_useful_typed_snps, p_m_all_snps,
                                p_m_typed_snps,values.last_sigma_it,values.row,
                                start_pos, delta);

				impz(p_maf_snps , p_ignore_snps , chrom , out_dir , last_name , p_snps_flag , values.weight ,typed_snps, snp_map , impute_flags ,p_useful_typed_snps );
				//final.start new recorder
				clean_all_vector(p_maf_snps , p_origin_typed_snps , p_typed_snps ,
								p_ignore_snps , p_snp_map , p_hap ,p_useful_typed_snps , p_snps_flag);

				if(1 == flag)
				{
					break;
				}
				record_all(p_origin_typed_snps , line_list[2] , line_list[3] , line_list[4] , line_list[5]);
			}
			else
			{
				record_all(p_origin_typed_snps , line_list[2] , line_list[3] , line_list[4] , line_list[5]);

				//record it into origin_typed_snps
			}
		}
		fin.close();

		for(int i = 0;i < (values.row);i++)
		{
			free(values.last_sigma_it[i]);
		}
		free(values.last_sigma_it);
		m_all_snps.clear();
		m_typed_snps.clear();
}

int main(int argc , char *argv[])
{
	char opt;
	char* Annotation;
	char* XQTL_path;
	char* Vcf_prefix;
	char* Out;
	char* BATCH = NULL;
	char* MAF = NULL;
	char* LAMBDA = NULL;
	char* WIN = NULL;
	char* CHR = NULL;
	char* EXCLUDE = NULL;
	char* EXCLUDE_FILE = NULL;
	char* SORT = NULL;
	char* DELTA = NULL;

	const char *const short_options = "hx:m:v:o:t:f:l:w:c:e:b:s:d:";
	const struct option long_options[] = {
	{"help", 0, NULL, 'h'},
	{"xQTL", 1, NULL, 'x'},
	{"molecule", 1, NULL, 'm'},
	{"VCF", 1, NULL, 'v'},
	{"output", 1, NULL, 'o'},
	{"threads", 1, NULL, 't'},
	{"MAF_cutoff", 1, NULL, 'f'},
	{"lambda", 1, NULL, 'l'},
	{"window_size", 1, NULL, 'w'},
	{"Chr",1,NULL,'c'},
	{"exclude" , 1 , NULL , 'e'},
	{"exclude_file" , 1  , NULL , 'b'},
	{"sort" , 1 , NULL , 's'},
	{"delta" , 1 , NULL , 'd'},
	{NULL, 0, NULL, 0 }
	};

	char *program_name;
	char input_filename[256], output_directory[256];

	int v = 0;
	strcpy(output_directory, "result");
	int next_option = 1;

	int counter = 0;
	program_name = argv[0];
	do {
		next_option = getopt_long (argc, argv, short_options, long_options, NULL);

		switch (next_option)
		 {
			case 'h':
				print_usage (stdout, 0);
				break;

			case 'm': // Molecular annotation file
				Annotation = (char *) strdup(optarg);
				break;
			case 'x': // xQTL statistic summary
				XQTL_path = (char *) strdup(optarg);
				break;
			case 'v': // genome reference panel, such as 1000G VCF
				Vcf_prefix = (char *) strdup(optarg);
				break;
			case 'o': // output folder path
				Out = (char *) strdup(optarg);
				break;
			case 't': // threads number， 1 in default
				BATCH = (char*)strdup(optarg);
				break;
			case 'f': // MAF threshold for variants in reference panel, 0.01 in default
				MAF = (char*)strdup(optarg);
				break;
			case 'l': // lambda value, 0.1 in default
				LAMBDA = (char*)strdup(optarg);
				break;
			case 'w': // window size in total(upstream and downstream of TSS), 500kb in default
				WIN = (char*)strdup(optarg);
				break;
			case 'c': // chrom
				CHR = (char*)strdup(optarg);
				break;
			case 'e'://region for imputation
				EXCLUDE = (char*)strdup(optarg);
				break;
			case 'b'://region for imputation
				EXCLUDE_FILE = (char*)strdup(optarg);
				break;
			case 's'://region for imputation
				SORT = (char*)strdup(optarg);
				break;
		     case 'd':
		         DELTA = (char*)strdup(optarg);
		         break;
			case '?':
				print_usage (stdout, 1);
				break;

			case -1:/* Done with options. */
				if(counter ==0)
				{
				        print_usage (stdout, 0);
				}
				break;

			default:/* Something else: unexpected. */
				print_usage (stderr, -1);
		 }
		counter++;
	} while (next_option != -1);

	string annotation = string(Annotation);
	string Xqtl_path = string(XQTL_path);
	string vcf_prefix = string(Vcf_prefix);
	string out = string(Out);
	string exclude =  "";
	string exclude_file = "";
	string sort_flag = "";
	int exclude_flag = 0;
	int exclude_file_flag = 0;
	int batch = 1;
	int chr = -1;
	double maf = 0.01;
	double lam = 0.1;
	int window_size = 500000;
	double delta = 0;

	if(EXCLUDE == NULL)
	{

	}
	else
	{
		exclude  = string(EXCLUDE);
		exclude_flag = 1;
	}

	if(EXCLUDE_FILE == NULL)
	{

	}
	else
	{
		exclude_file = string(EXCLUDE_FILE);
		exclude_file_flag = 1;
	}


	if(BATCH == NULL)
	{
		batch = 1;
	}
	else
	{
		batch = atoi(BATCH);
	}

	if(MAF == NULL)
	{
		maf = 0.01;
	}
	else
	{
		maf = double(atof(MAF));
	}

	if(LAMBDA == NULL)
	{
		lam = 0.1;
	}
	else
	{
		lam = double(atof(LAMBDA));
	}

	if(WIN == NULL)
	{
		window_size = 500000;
	}
	else
	{
		window_size = int(atoi(WIN));
	}


	if(CHR == NULL)
	{
		chr = -1;
	}
	else
	{
		chr = int(atoi(CHR));
	}

	if(SORT == NULL)
	{
        	sort_flag = "FALSE";
	}
	else
	{
		sort_flag = string(SORT);
	}

    if(!(sort_flag == "TRUE" || sort_flag == "FALSE"))
    {
        cout << "wrong parameter for --sort/-s\n";
        exit(0);
    }

    if(DELTA == NULL)
    {
        delta = 0.0;
    }
    else
    {
        delta = double(atof(DELTA));
    }

	//load pos_map
	cout << "Loading molecular annotation file ... ";
	map<string,long long int*> pos1;
	map<string,long long int*> *pos_map = &pos1;
	long long int anno_num = load_pos_map(annotation , pos_map);
	 cout << "Done!" << endl;
	//seperate chrom
	long long int chrom[1261];
	if("FALSE" == sort_flag)
	{
		//do nothing
	}
	else
	{
		cout << "Reorganizing xQTL file ... ";
		reorganize_xqtl(anno_num , Xqtl_path , pos_map , chr , out);
		cout << "Done!" << endl;
		if(chr == -1)
		{
			Xqtl_path = out + ".input_xQTL_name.sorted.tmp";
		}
		else
		{
			Xqtl_path = out + "." + string(CHR) + "/.input_xQTL_name.sorted.chr.tmp";
		}

	}
	int chrom_num = split_chrom(Xqtl_path , chrom);
	int total_num = 0;
	for(int j = 1;j <= chrom_num;j++)
	{
		if(chrom[j] != chrom[j - 1])
		{
			total_num++;
		}
	}

	cout << total_num << " chromosomes detected!" << endl;

	make_output_dir(chrom , chrom_num , Out , chr);



	for(int i = 1;i <= chrom_num;i++)
	{
		if(chr != -1)
		{
			if(chr == i)
			{
				//pass
			}
			else
			{
				continue;
			}
		}
		long long int start = chrom[i - 1];
		long long int end = chrom[i];

		if(start == end)
		{
			continue;
		}

		//load pos_file_map
		printf("Loading reference VCF file of chromosome No.%d ... ",i);
		map<string,long long int> VcfIndex;
		map<string,long long int>* p_VcfIndex = &VcfIndex;

		vector<string> VcfFile;
		vector<string>* p_VcfFile = &VcfFile;

		char tem[3];
		sprintf(tem , "%d" , i);
		string ref_file = vcf_prefix;
		bool load = false;

		if(exclude_flag == 1)
		{
			load = gzLoadVcfFile_exclude( exclude , tem , ref_file.c_str() ,p_VcfIndex ,p_VcfFile );
		}
		else if(exclude_file_flag == 1)
		{
			load = gzLoadVcfFile_exclude_file(exclude_file , tem , ref_file.c_str() ,p_VcfIndex ,p_VcfFile );
		}
		else
		{
			load = gzLoadVcfFile(tem , ref_file.c_str() ,p_VcfIndex ,p_VcfFile );
		}

		cout << "Done!\n";

		/////////////////////////////////////////

		vector<long long int> batch_bonder;
		vector<long long int>* p_batch_bonder = &batch_bonder;

		vector<string> files;
		vector<string>* p_files = &files;

		int real_batch = travel_Xqtl(p_files , start ,end ,Xqtl_path ,p_batch_bonder , batch);
		/////////////////////////////////////////

		struct thread_data td[MAX_THREAD_NUM];
		thread tasks[MAX_THREAD_NUM];
		int chrom = i;
		printf("Performing xQTL imputation on chromosome No.%d ...\n",chrom);
		printf("Partitioning into %d threads ... \n" , batch);
		printf("Running imputation ... \n");
		for(int ii = 0;ii < real_batch;ii++)
		{
			td[ii].pos_map = pos_map;
			td[ii].p_VcfIndex =  p_VcfIndex;
			td[ii].p_VcfFile =  p_VcfFile;
			td[ii].Xqtl_path = Xqtl_path;
			td[ii].ref_file = ref_file;
			td[ii].start = batch_bonder[2 * ii];
			td[ii].end = batch_bonder[2 * ii + 1];
			td[ii].out_dir = out;
			td[ii].chrom = chrom;
			td[ii].maf = maf;
			td[ii].lam = lam;
			td[ii].window_size = window_size;
			td[ii].delta = delta;
			tasks[ii] = thread(main_process , td + ii ,ii);
		}
	// To make sure all sub-threads are finished in each chromosome, because they share the same memory of the reference panel

		for(int j = 0;j < real_batch;j++)
    		{
			if(tasks[j].joinable())
			{
				tasks[j].join();
			}
    		}

		printf("Imputation on chromosome No.%d finished!\n",chrom);
		printf("Finalizing output files ... ");
		organize_files(p_files , chrom , out , pos1);
		cout << "Done!" << endl;
	}


	cout << "Imputation processes have been Successfully Finished!\n";
	if("TRUE" == sort_flag)
	{
		if(chr == -1)
		{
			remove(Xqtl_path.c_str());
		}
		else
		{
			remove(Xqtl_path.c_str());
			string out_dir = out + "." + string(CHR);
			remove(out_dir.c_str());
		}
	}


	return 0;
}
