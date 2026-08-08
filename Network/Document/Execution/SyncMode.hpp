#pragma once
#include <score_addon_network_export.h>
#include <QByteArray>
namespace Network
{
enum class ExpressionPolicy
{
  OnFirst,
  OnMajority,
  OnAll
};

enum class SyncMode
{
  NonCompensatedSync,
  CompensatedSync,
  NonCompensatedAsync,
  CompensatedAsync
};

enum class ShareMode
{
  Shared,
  Mixed,
  Free
};

struct SCORE_ADDON_NETWORK_EXPORT MessagesAPI
{
  MessagesAPI();
  static const MessagesAPI& instance();

  const QByteArray command_new;
  const QByteArray command_undo;
  const QByteArray command_redo;
  const QByteArray command_index;
  //! Master -> the client whose command it could not apply. That client is now
  //! the one out of sync with the session, so it marks itself diverged.
  const QByteArray command_rejected;
  //! A question addressed to one peer, and its answer. Alongside the document
  //! channel rather than part of it: nothing here changes the document.
  const QByteArray rpc_request;
  const QByteArray rpc_response;

  const QByteArray lock;
  const QByteArray unlock;

  //! Host -> peers: whether one of the score's devices is connected. A peer
  //! that does not run the score has nothing to ask.
  const QByteArray device_status;
  //! Peer -> host: "set this", which the host carries out on the device.
  const QByteArray device_value;
  //! Host -> peers: "this is now the value". A notification, not a request:
  //! applying it as a request would send it straight back where it came from.
  const QByteArray device_value_changed;
  const QByteArray device_tree;

  //! Host -> peers: how far along the score is, so that a peer with no
  //! executor can still show where it has got to.
  const QByteArray exec_position;

  const QByteArray ping;
  const QByteArray pong;
  const QByteArray play;
  const QByteArray stop;

  const QByteArray session_portinfo;
  const QByteArray session_askNewId;
  const QByteArray session_idOffer;
  const QByteArray session_join;
  const QByteArray session_document;
  //! Master -> a client it will not accept, carrying the reason.
  const QByteArray session_rejected;

  const QByteArray trigger_expression_true;
  const QByteArray trigger_previous_completed;
  const QByteArray trigger_entered;
  const QByteArray trigger_left;
  const QByteArray trigger_finished;
  const QByteArray trigger_triggered;

  const QByteArray trigger_triggered_compensated;

  const QByteArray interval_speed;

  const QByteArray netpit_in_message;
  const QByteArray netpit_out_message;
  const QByteArray netpit_in_audio;
  const QByteArray netpit_out_audio;
  const QByteArray netpit_in_video;
  const QByteArray netpit_out_video;
  const QByteArray netpit_in_geometry;
  const QByteArray netpit_out_geometry;
};
}
