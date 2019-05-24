#include "hero.h"

static xml_ctx_t* __dsa_heros_ctx_from_src( archive_resource_t *archive, const char *resname) {
    
    xml_ctx_t* result = NULL;

    if ( archive != NULL && resname != NULL && ( strlen(resname) > 0 )) {
        const xml_source_t* src = xml_source_from_resname(archive, resname);

        if (src != NULL) {
            result = xml_ctx_new(src);
        }
	    
    }

    return result;
} 

static char * __dsa_heros_get_next_id(dsa_heros_t *heros) {
    char *maxid = NULL;

    xmlXPathObjectPtr result = xml_ctx_xpath(heros->heros, "/heros/hero[@id = max(/heros/hero/@id)]");

    if (result && result->nodesetval && result->nodesetval->nodeNr == 1) {
       maxid = xmlGetProp(result->nodesetval->nodeTab[0] ,"id");
       long long maxval = atoll(maxid);
       xmlFree(maxid);
       maxid = format_string_new("%llu", ++maxval);
    } 

    xmlXPathFreeObject(result);

    return maxid;
}

void dsa_heros_free(dsa_heros_t **heros) {

    if(heros != NULL && *heros != NULL) {
        dsa_heros_t *delete_heros = *heros;
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->basehero));
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->breeds));
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->cultures));
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->professions));
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->pro_contra));
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->specialabilities));
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->spells));
        free_xml_ctx_src(((xml_ctx_t **)&delete_heros->talents));
        free_xml_ctx_src(&delete_heros->heros);

        free(delete_heros);
        *heros = NULL;
    }
}

dsa_heros_t* dsa_heros_new_archiv(archive_resource_t *archive) {

    dsa_heros_t _temp_heros = {
        __dsa_heros_ctx_from_src(archive, "basehero"),
        __dsa_heros_ctx_from_src(archive, "breeds"),
        __dsa_heros_ctx_from_src(archive, "cultures"),
        __dsa_heros_ctx_from_src(archive, "professions"),
        __dsa_heros_ctx_from_src(archive, "procontra"),
        __dsa_heros_ctx_from_src(archive, "specialabilities"),
        __dsa_heros_ctx_from_src(archive, "spells"),
        __dsa_heros_ctx_from_src(archive, "talents"),
        xml_ctx_new_empty_root_name("heros")
    };

    dsa_heros_t *newheros = malloc(sizeof(dsa_heros_t));

    memcpy(newheros, &_temp_heros, sizeof(dsa_heros_t));

    return newheros;
}

bool dsa_heros_load(dsa_heros_t *heros, const char *filename) {
    xml_ctx_t *nCtx = xml_ctx_new_file(filename);
    free_xml_ctx(&heros->heros);
    heros->heros = nCtx;
    return ( heros->heros->state.state_no == XML_CTX_SUCCESS);
}

bool dsa_heros_save(dsa_heros_t *heros, const char *filename) {
    xml_ctx_save_file(heros->heros, filename);
    return ( heros->heros->state.state_no == XML_CTX_SUCCESS);
}

dsa_hero_t* dsa_hero_new(dsa_heros_t *heros, const char* hero_name) {

    dsa_hero_t *new_hero = NULL;

    if ( regex_match("[\\d\\D]+",hero_name) ) {

        xml_ctx_t *nCtx = xml_ctx_new_empty();
    	xmlNodePtr newheroroot = xmlCopyNode(xmlDocGetRootElement(heros->basehero->doc), 1);
        
        xmlSetProp(newheroroot, "name", hero_name);
        
        char * id = __dsa_heros_get_next_id(heros);

        xmlSetProp(newheroroot, "id", (id == NULL ? "0" : id ));

        #if debug > 0
            printf("create new hero: name \"%s\" with id \"%s\"\n", hero_name, (id == NULL ? "0" : id ));
        #endif

        free(id);

        xmlDocSetRootElement(nCtx->doc, newheroroot);
        
        new_hero = malloc(sizeof(dsa_hero_t));
        new_hero->xml = nCtx;

        xmlNodePtr herosroot = xmlDocGetRootElement(heros->heros->doc);

        xmlAddChild(herosroot, xmlCopyNode(newheroroot, 1));
    }

    return new_hero;
}

dsa_hero_t* dsa_hero_free(dsa_hero_t **hero) {
    if (hero != NULL && *hero != NULL) {
        dsa_hero_t *todelete_hero = *hero;
        free_xml_ctx_src(&todelete_hero->xml);
        free(todelete_hero);
        *hero = NULL;
    }
}

//returns a copy of found hero by id. Id is needed because names are redudant.
dsa_hero_t* dsa_hero_get(dsa_heros_t *heros, const int id) {
    
    xmlXPathObjectPtr result = xml_ctx_xpath_format(heros->heros, "/heros/hero[@id='%i']", id);

    dsa_hero_t *found_hero = NULL;

    if (result && result->nodesetval && result->nodesetval->nodeNr > 0) {
        xmlNodePtr found = result->nodesetval->nodeTab[0];

        found_hero = malloc(sizeof(dsa_hero_t));
        found_hero->xml = xml_ctx_new_node(found);
    }

    xmlXPathFreeObject(result);

    return found_hero;
}

//returns the pointer of first hero or NULL if no hero was found
//the last pointer element is set to NULL for easier iteration.
dsa_hero_entry_t* dsa_hero_get_all(dsa_heros_t *heros) {
    
    xmlXPathObjectPtr result = xml_ctx_xpath(heros->heros, "/heros/hero");

    dsa_hero_entry_t *herolist = NULL;

    if (result && result->nodesetval && result->nodesetval->nodeNr > 0) {
        

        xmlNodeSetPtr nodes = result->nodesetval;
        herolist = malloc((result->nodesetval->nodeNr+1) * sizeof(dsa_hero_entry_t));

        #if debug > 0
            printf("found entries: %i\n", result->nodesetval->nodeNr);
        #endif

        for(int curnode = 0; curnode < nodes->nodeNr; ++curnode) {

            xmlNodePtr found = nodes->nodeTab[curnode];

            dsa_hero_entry_t *hero_entry = &herolist[curnode];
            hero_entry->name = xmlGetProp(found, "name");
            char *id = xmlGetProp(found, "id");
            hero_entry->id = atoll(id);
            free(id);

        }

        herolist[nodes->nodeNr].name = NULL;
        herolist[nodes->nodeNr].id = -1;

    }

    xmlXPathFreeObject(result);

    return herolist;
}

void dsa_hero_list_free(dsa_hero_entry_t **hero_list) {

    dsa_hero_entry_t *todelete_hero_list = *hero_list;
    dsa_hero_entry_t *cur;

	int cnt = 0;
	cur = todelete_hero_list;
	while( cur->name != NULL && cur->id != -1 ) {
        xmlFree(cur->name);
		cnt++;
		cur++;
	}
    free(todelete_hero_list);
    *hero_list = NULL;
}

//saves as replacement the hero inside of heros(not to file) by searching the ID
void dsa_heros_save_hero(dsa_heros_t *heros, const dsa_hero_t *hero) {

}