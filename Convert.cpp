#include "Convert.h"

Graph<int> Convert::BuildGraph(const Data& data) {
    Graph<int> g;

    // Add one vertex per web
    for (const auto& web : data.webs)
        g.addVertex(web.id);

    // Add an edge between any two webs that interfere
    for (int i = 0; i < (int)data.webs.size(); i++)
        for (int j = i + 1; j < (int)data.webs.size(); j++)
            if (data.webs[i].OverlapCheck(data.webs[j])) {
                g.addEdge(data.webs[i].id, data.webs[j].id, 1);
                g.addEdge(data.webs[j].id, data.webs[i].id, 1); // undirected
            }
    return g;
}