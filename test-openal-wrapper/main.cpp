#include "SoundObj.hpp"

#include <chrono>
#include <thread>
#include <iostream>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
namespace bmi = boost::multi_index;
using SoundMap = bmi::multi_index_container<
SoundObj*,
bmi::indexed_by<
bmi::hashed_unique<
bmi::const_mem_fun<
SoundObj,
const std::string&,
&SoundObj::name
>
>
>
>;
class Scene : public QObject
{
    public:
        Scene()
        {
            // create a listener and set its position, velocity and orientation
            m_listener.Position(0.0f, 0.0f, 0.0f);
            m_listener.Velocity(0.0f, 0.0f, 0.0f);
            m_listener.Orientation(0.0f, 0.0f,-1.0f, 0.0f, 1.0f, 0.0f);


        }

        void start()
        {
            for(auto& cst_sound : m_sounds)
            {
                cst_sound->sound.source().Play();
            }
        }

        oalplus::Listener& listener()
        { return m_listener; }

        SoundMap& sounds()
        { return m_sounds; }

    private:
        oalplus::Listener m_listener;
        SoundMap m_sounds;
};

#include <Network/Device.h>
#include <Network/Protocol.h>
#include <Network/Node.h>
#include <Editor/Domain.h>

template<typename Get, typename Set>
struct Parameter
{
        Get get;
        Set set;
};


template<typename Get, typename Set>
auto make_parameter(Get&& get, Set&& set)
{
    return Parameter<Get, Set>{std::move(get), std::move(set)};
}

template<typename Fun>
auto add_child(std::shared_ptr<OSSIA::Node>& node,
               const std::string& name,
               OSSIA::Value::Type type,
               Fun&& callback)
{
    auto new_node = *node->emplace(node->children().cend(), name);
    new_node->createAddress(type)->setValueCallback(callback);

    return new_node;
}

template<typename Fun>
auto add_float_child(std::shared_ptr<OSSIA::Node>& node,
               const std::string& name,
               Fun&& callback)
{
    auto new_node = *node->emplace(node->children().cend(), name);
    auto addr = new_node->createAddress(OSSIA::Value::Type::FLOAT);

    addr->setValueCallback(callback);
    addr->setDomain(OSSIA::Domain::create(new OSSIA::Float(-100.), new OSSIA::Float(100.)));
    return new_node;
}

template<typename Parameter_t> // a function that returns a Vec3f& to modify.
void add_position(std::shared_ptr<OSSIA::Node> node, Parameter_t&& param)
{
    static auto modify_i = [=] (int i) {
        return [=] (const OSSIA::Value* val) {
            auto float_val = dynamic_cast<const OSSIA::Float*>(val);
            if(!float_val)
                return;

            auto elt = param.get();
            elt[i] = float_val->value;
            param.set(elt);
        };
    };

    auto addr = node->createAddress(OSSIA::Value::Type::TUPLE); // [ x y z ]

    addr->setValueCallback([=] (const OSSIA::Value* val) {
        auto tpl = dynamic_cast<const OSSIA::Tuple*>(val);
        if(tpl->value.size() != 3)
            return;

        auto x = dynamic_cast<const OSSIA::Float*>(tpl->value[0]);
        auto y = dynamic_cast<const OSSIA::Float*>(tpl->value[1]);
        auto z = dynamic_cast<const OSSIA::Float*>(tpl->value[2]);
        if(!x || !y || !z)
            return;

        param.set(oalplus::Vec3f{x->value, y->value, z->value});
    });

    add_float_child(node, "x", modify_i(0));
    add_float_child(node, "y", modify_i(1));
    add_float_child(node, "z", modify_i(2));
}

std::shared_ptr<OSSIA::Node> getNode(std::shared_ptr<OSSIA::Node> parent, 
								     const std::string& name)
{
	auto it = boost::range::find_if(
				parent->children(),
				[&] (const auto& node) { return node->getName() == name; });
	Q_ASSERT(it != parent->children().end());
	return *it;
}

#include <thread>
#include <chrono>
class RemoteSceneManager
{
        Scene m_scene;
        std::shared_ptr<OSSIA::Device> m_dev;
        std::shared_ptr<OSSIA::Node> m_sourcesNode;
        std::shared_ptr<OSSIA::Node> m_sourcesListNode;

    private:
        //// Global settings ////
        void setupGlobal()
        {
            auto settings_node = *m_dev->emplace(m_dev->children().cend(), "settings");
            auto vol_node = *settings_node->emplace(settings_node->children().cend(), "volume");
            auto files_node = *settings_node->emplace(settings_node->children().cend(), "files");

            auto vol_addr = vol_node->createAddress(OSSIA::Value::Type::FLOAT);
            auto files_addr = files_node->createAddress(OSSIA::Value::Type::TUPLE);

            vol_addr->setValueCallback([] (const OSSIA::Value* val) { });
            files_addr->setValueCallback([] (const OSSIA::Value* val) { });
        }

        //// Listener settings ////
        void setupListener()
        {
            auto listener_node = *m_dev->emplace(m_dev->children().cend(), "listener");

            auto listener_pos_node = *listener_node->emplace(listener_node->children().cend(), "pos");

            add_position(listener_pos_node,
                         make_parameter(
                             [&] () { return m_scene.listener().Position(); },
                             [&] (const auto& elt) { m_scene.listener().Position(elt); }));

            auto listener_orient_node = *listener_node->emplace(listener_node->children().cend(), "orientation");
            auto listener_orient_addr = listener_orient_node->createAddress(OSSIA::Value::Type::TUPLE); // [ at_x at_y at_z up_x at_y at_z ]
            listener_orient_addr->setValueCallback([&] (const OSSIA::Value* val) {
                auto tpl = dynamic_cast<const OSSIA::Tuple*>(val);
                if(tpl->value.size() != 6)
                    return;

                auto at_x = dynamic_cast<const OSSIA::Float*>(tpl->value[0]);
                auto at_y = dynamic_cast<const OSSIA::Float*>(tpl->value[1]);
                auto at_z = dynamic_cast<const OSSIA::Float*>(tpl->value[2]);
                auto up_x = dynamic_cast<const OSSIA::Float*>(tpl->value[3]);
                auto up_y = dynamic_cast<const OSSIA::Float*>(tpl->value[4]);
                auto up_z = dynamic_cast<const OSSIA::Float*>(tpl->value[5]);
                if(!at_x || !at_y || !at_z || !up_x || !up_y || !up_z)
                    return;

                m_scene.listener().Orientation(at_x->value, at_y->value, at_z->value, up_x->value, up_y->value, up_z->value);
            });

            auto listener_orient_at_node = *listener_orient_node->emplace(listener_orient_node->children().cend(), "at");
            add_position(listener_orient_at_node,
                         make_parameter(
                             [&] () { return m_scene.listener().OrientationAt(); },
                             [&] (const auto& elt) { m_scene.listener().Orientation(elt, m_scene.listener().OrientationUp()); }
            ));
            auto listener_orient_up_node = *listener_orient_node->emplace(listener_orient_node->children().cend(), "up");
            add_position(listener_orient_up_node,
                         make_parameter(
                             [&] () { return m_scene.listener().OrientationUp(); },
                             [&] (const auto& elt) { m_scene.listener().Orientation(m_scene.listener().OrientationAt(), elt); }
            ));
        }

        //// Sources settings ////
        void setupSources()
        {
            m_sourcesNode = *m_dev->emplace(m_dev->children().cend(), "sources");
            m_sourcesListNode = *m_sourcesNode->emplace(m_sourcesNode->children().cend(), "sources");

            add_child(m_sourcesNode, "add", OSSIA::Value::Type::STRING,
                      [&] (const OSSIA::Value* val) { on_sourceAdded(val);} );

            add_child(m_sourcesNode, "remove", OSSIA::Value::Type::STRING,
                      [&] (const OSSIA::Value* val) { on_sourceRemoved(val);});
        }

        void on_sourceAdded(const OSSIA::Value* val)
        {
            auto str_val = dynamic_cast<const OSSIA::String*>(val);
            if(!str_val)
                return;

            // Create the sound
            auto sound_obj = new SoundObj{str_val->value};
            sound_obj->setParent(&m_scene);
            m_scene.sounds().insert(sound_obj);
            auto& sound = sound_obj->sound;

            // Create the callbacks and OSC device commands
            auto src_node = *m_sourcesListNode->emplace(m_sourcesListNode->children().cend(), str_val->value);

            // Position
            auto src_pos_node = *src_node->emplace(src_node->children().cend(), "pos");
            add_position(src_pos_node,
                         make_parameter(
                             [&] () { return sound.source().Position(); },
                             [&] (const auto& elt) { sound.source().Position(elt); }
            ));

            // Enablement
            add_child(src_node, "enabled", OSSIA::Value::Type::BOOL,
                      [&,sound_obj] (const OSSIA::Value* val) {
                auto enablement_val = dynamic_cast<const OSSIA::Bool*>(val);
                if(!enablement_val)
                    return;
                sound_obj->enablementChanged(enablement_val->value);
            });

            // Audio file
            add_child(src_node, "file", OSSIA::Value::Type::STRING,
                      [&,sound_obj] (const OSSIA::Value* val) {
                auto filename_val = dynamic_cast<const OSSIA::String*>(val);
                if(!filename_val)
                    return;

                sound_obj->fileChanged(QString::fromStdString(filename_val->value));
            });
        }

        void on_sourceRemoved(const OSSIA::Value* val)
        {
            auto str_val = dynamic_cast<const OSSIA::String*>(val);
            if(!str_val)
                return;

            // Remove from the sounds
            m_scene.sounds().erase(str_val->value);

            // Remove from the OSC device
            auto& children = m_sourcesNode->children();
            auto it = boost::range::find_if(children,
                                            [&] (auto&& elt) { return elt->getName() == str_val->value; });
            if(it != children.end())
                children.erase(it);
        }

    public:
        RemoteSceneManager(std::shared_ptr<OSSIA::Device> ptr):
            m_dev{ptr}
        {
            setupGlobal();
            setupListener();
            setupSources();

            on_sourceAdded(new OSSIA::String("ASound"));
            auto node = getNode(m_sourcesListNode, "ASound");

            auto enabled = getNode(node, "enabled");
            enabled->getAddress()->sendValue(new OSSIA::Bool(true));

            auto file = getNode(node, "file");
            file->getAddress()->sendValue(new OSSIA::String("snd1.wav"));

            /*
            new std::thread([&] ()
            {
                while(true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    auto pos = m_scene.listener().Position();
                    pos[1] += 5;
                    qDebug() << pos.y();

                    m_scene.listener().Position(pos);
                }
            });
            */
        }

        void start() { return m_scene.start(); }
};



#include <QApplication>
int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    oalplus::Device device;
    oalplus::ContextMadeCurrent context(
                device,
                oalplus::ContextAttribs().Add(oalplus::ContextAttrib::MonoSources, 1).Get());

    RemoteSceneManager s{OSSIA::Device::create(OSSIA::Local {}, "MinuitDevice")};
    auto iscore_dev = OSSIA::Device::create(OSSIA::Minuit{"127.0.0.1", 13579, 9998}, "i-score");
    (void) iscore_dev;

    s.start();

    return app.exec();
}
