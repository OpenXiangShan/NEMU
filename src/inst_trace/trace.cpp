#include "inst_dep_record.pb.h"
#include "packet.pb.h"
#include "trace.h"


void ElasticTrace::init(const char *data_file, const char *inst_file)
{
    assert(data_file);
    assert(inst_file);
    assert(strcmp(data_file, "") != 0);
    assert(strcmp(inst_file, "") != 0);
    instTraceStream = new ProtoOutputStream(inst_file);
    dataTraceStream = new ProtoOutputStream(data_file);

    ProtoMessage::PacketHeader inst_pkt_header;
    inst_pkt_header.set_obj_id("nemu_fetch");
    inst_pkt_header.set_tick_freq(2UL*1024*1024*1024); // 2G
    instTraceStream->write(inst_pkt_header);

    ProtoMessage::InstDepRecordHeader data_rec_header;
    data_rec_header.set_obj_id("nemu_data");
    data_rec_header.set_tick_freq(2UL*1024*1024*1024); // 2G
    data_rec_header.set_window_size(200);
    dataTraceStream->write(data_rec_header);
}


void ElasticTrace::recordMem(uint64_t pc, uint64_t paddr)
{
    ProtoMessage::InstDepRecord dep_pkt;
    dep_pkt.set_type(ProtoMessage::InstDepRecord_RecordType::InstDepRecord_RecordType_LOAD);
    dep_pkt.set_seq_num(instCount++);
    dep_pkt.set_pc(pc);
    dep_pkt.set_flags(0);
    dep_pkt.set_p_addr(paddr);
    dep_pkt.set_size(1);
    dep_pkt.set_comp_delay(1);
    dataTraceStream->write(dep_pkt);
    tick += 500;
}


void ElasticTrace::recordFetch(uint64_t pc, uint64_t inst_paddr)
{
    ProtoMessage::Packet inst_fetch_pkt;
    inst_fetch_pkt.set_tick(tick);
    inst_fetch_pkt.set_cmd(1);  // MemCmd::ReadReqinstCount
    inst_fetch_pkt.set_pc(pc);
    inst_fetch_pkt.set_flags(0);
    inst_fetch_pkt.set_addr(inst_paddr);
    inst_fetch_pkt.set_size(2); // RVC minimal size
    instTraceStream->write(inst_fetch_pkt);
    tick += 500;
}

void ElasticTrace::close()
{
    if (dataTraceStream) {
        assert(instTraceStream);
        delete dataTraceStream;
        delete instTraceStream;
    }
}

ElasticTrace elasticTracer;

extern "C" {

void init_tracer(const char *data_file, const char *inst_file)
{
    elasticTracer.init(data_file, inst_file);
}

void recordMem(uint64_t pc, uint64_t paddr)
{
    elasticTracer.recordMem(pc, paddr);
}

void recordFetch(uint64_t pc, uint64_t inst_paddr)
{
    elasticTracer.recordFetch(pc, inst_paddr);
}

void close_tracer()
{
    elasticTracer.close();
}

}