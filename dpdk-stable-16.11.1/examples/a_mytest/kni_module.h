#ifndef KNI_H
#define KNI_H

/* Parse the arguments given in the command line of the application */
extern int parse_args(int argc, char **argv);
extern void init_kni(void);
extern void init_port(uint8_t port);
extern int kni_alloc(uint8_t port_id);
extern int kni_free_kni(uint8_t port_id);
extern void kni_burst_free_mbufs(struct rte_mbuf **pkts, unsigned num);
/* Print out statistics on packets handled */
extern void print_stats(void);
#endif  //end of kni_module.h