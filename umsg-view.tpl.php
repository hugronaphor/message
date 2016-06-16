<?php
//dsm(get_defined_vars());
?>
<div <?php if (!empty($message_classes)) { ?>class="<?php echo implode(' ', $message_classes); ?>" <?php } ?> id="umsg-mid-<?php print $mid; ?>">

  <div class="umsg-message-teaser">

    <div class="click umsg-author-info">
      <div class="umsg-author-avatar">
        <?php print $author_picture; ?>    
      </div>
      <span class="umsg-author-name"><?php print $author_name_link; ?></span>
    </div>

    <div class="click umsg-message-date">
      <?php print $message_timestamp; ?>
    </div>

    <div class="umsg-message-body-short">
      <?php print $message_body_short; ?>
    </div>
    <div class="hide store-short-body"><?php print $message_body_short; ?></div>
  </div>

  <div class="hide umsg-message-additional">

    <?php if (isset($message_actions)): ?>
    <div class="hide umsg-action-btns"><?php print $message_actions ?></div>
    <?php endif ?>

    <div class="umsg-message-body">
      <?php print $message_body; ?>
    </div>
    
    <div class="umsg-reply-form"></div>

  </div>


  <div class="clearfix"></div>
</div>
