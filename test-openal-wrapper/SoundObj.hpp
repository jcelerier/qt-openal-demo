#pragma once
#include <QObject>
#include <QDebug>
#include <oalplus/al.hpp>
#include <oalplus/all.hpp>
#include <oalplus/alut.hpp>

class Sound
{
    public:
        Sound(Sound&&) = default;
        Sound(const std::string& name):
            m_name{name},
            m_source{oalplus::ObjectDesc(std::string(name))}
        {
            m_source.Looping(true);
            m_source.ReferenceDistance(15);
        }

        auto& source()
        {
            return m_source;
        }

        const auto& source() const
        {
            return m_source;
        }

        void setEnabled(bool b)
        {
            m_source.Gain(b ? m_gain : 0.);
        }

        const std::string& name() const
        {
            return m_name;
        }

        void load(const std::string& file)
        {
            m_source.Buffer(oalplus::ALUtilityToolkit(false).CreateBufferFromFile(file));
            m_source.Play();
        }

    private:
        std::string m_name;
        oalplus::Source m_source;
        double m_gain{1.};
};

class SoundObj : public QObject
{
        Q_OBJECT
    public:
        SoundObj(const std::string& name):
            sound{name}
        {
            // Why doesn't queued work ?
            connect(this, &SoundObj::enablementChanged, this, &SoundObj::on_enablementChanged, Qt::DirectConnection);
            connect(this, &SoundObj::fileChanged, this, &SoundObj::on_fileChanged, Qt::DirectConnection);
        }

        virtual ~SoundObj() = default;

        const std::string& name() const
        { return sound.name(); }

        Sound sound;

    signals:
        void enablementChanged(bool);
        void fileChanged(QString);

    public slots:
        void on_enablementChanged(bool b)
        {
            sound.setEnabled(b);
        }

        void on_fileChanged(QString str)
        {
            sound.load(str.toStdString());
        }

};
